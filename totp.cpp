#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace {
std::vector<uint8_t> base32Decode(const std::string& encoded) {
    std::vector<uint8_t> ret;

    unsigned int curByte = 0;
    int bits = 0;

    for (auto ch : encoded) {
        if (ch >= 'A' && ch <= 'Z') {
            ch -= 'A';
        } else if (ch >= '2' && ch <= '7') {
            ch -= '2';
            ch += 26;
        }

        curByte = (curByte << 5) | ch;
        bits += 5;

        if (bits >= 8) {
            ret.push_back((curByte >> (bits - 8)) & 255);
            bits -= 8;
        }
    }

    return std::move(ret);
}

class HMACWrapper {
private:
    std::function<void(HMAC_CTX*)> hmacFreeFn = [](HMAC_CTX* ptr) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        abort();
#else
        HMAC_CTX_free(ptr);
#endif
    };

public:
    HMACWrapper(const EVP_MD* digest, const std::vector<uint8_t>& key) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        ctx = std::unique_ptr<HMAC_CTX>(new HMAC_CTX);
        HMAC_CTX_init(ctx.get());
#else
        ctx = std::unique_ptr<HMAC_CTX, decltype(hmacFreeFn)>(HMAC_CTX_new(), hmacFreeFn);
#endif
        HMAC_Init_ex(ctx.get(), key.data(), key.size(), digest, nullptr);
    }

    void update(const std::vector<uint8_t>& s) {
        HMAC_Update(ctx.get(),
                    reinterpret_cast<const unsigned char*>(s.data()),
                    static_cast<unsigned int>(s.size()));
    }

    std::vector<uint8_t> finalize() {
        std::vector<uint8_t> finalHash(HMAC_size(ctx.get()));
        unsigned int size;
        HMAC_Final(ctx.get(), finalHash.data(), &size);
        if (size != finalHash.size()) {
            throw std::runtime_error("Overflow while finalizing HMAC");
        }
        return std::move(finalHash);
    }

private:
    std::unique_ptr<HMAC_CTX, decltype(hmacFreeFn)> ctx;
};

std::string calculateTOTPInternal(const EVP_MD* algo,
                                  uint64_t counter,
                                  int digits,
                                  const std::vector<uint8_t>& key) {
    std::vector<uint8_t> counterArr(8);
    for (int i = 7; i >= 0; i--) {
        counterArr[i] = counter & 0xff;
        counter >>= 8;
    }

    HMACWrapper hmac(algo, key);
    hmac.update(counterArr);
    auto finalHmac = hmac.finalize();

    const auto offset = finalHmac[19] & 0xf;
    uint32_t truncated = (finalHmac[offset] & 0x7f) << 24 | (finalHmac[offset + 1] & 0xff) << 16 |
        (finalHmac[offset + 2] & 0xff) << 8 | (finalHmac[offset + 3] & 0xff);

    std::stringstream ss;
    const int digitsPow = pow(10, digits);
    ss << std::setw(digits) << std::setfill('0') << truncated % digitsPow;
    return ss.str();
}

const auto kURIPattern = "otpauth://(totp|hotp)/(?:[^\\?]+)\\?((?:(?:[^=]+)=(?:[^\\&]+)\\&?)+)";
const auto kURIRegex = std::regex(kURIPattern, std::regex::ECMAScript | std::regex::optimize);
const auto kQueryComponentPattern = "([^=]+)=([^\\&]+)&?";
const auto kQueryComponentRegex =
    std::regex(kQueryComponentPattern, std::regex::ECMAScript | std::regex::optimize);

}  // namespace

bool isTOTPURI(const std::string& uri) {
    std::smatch match;
    return std::regex_match(uri, match, kURIRegex);
}

std::string calculateTOTP(const std::string& uri) {
    std::smatch match;
    if (!std::regex_match(uri, match, kURIRegex)) {
        throw std::runtime_error("Error parsing OTP URI");
    }

    const auto type = match[1].str();
    if (type != "totp") {
        std::stringstream ss;
        ss << "Unsupported OTP format: " << type;
        throw std::runtime_error(ss.str());
    }

    const auto query = match[2].str();
    auto begin = std::sregex_iterator(query.begin(), query.end(), kQueryComponentRegex);
    const auto end = std::sregex_iterator();
    std::unordered_map<std::string, std::string> params;
    for (auto it = begin; it != end; ++it) {
        auto curParam = *it;
        params.emplace(curParam[1], curParam[2]);
    }

    if (params.find("secret") == params.end()) {
        throw std::runtime_error("OTP URI is missing a key");
    }

    const EVP_MD* algo = EVP_sha1();
    if (params.find("algorithm") != params.end()) {
        const auto& algoStr = params["algorithm"];
        if (algoStr == "SHA256") {
            algo = EVP_sha256();
        } else if (algoStr == "SHA512") {
            algo = EVP_sha512();
        } else if (algoStr != "SHA1") {
            std::stringstream ss;
            ss << "Unsurpported algorithm in OTP URI: " << algoStr;
            throw std::runtime_error(ss.str());
        }
    }

    auto now = std::time(nullptr);
    int period = 30;
    if (params.find("period") != params.end()) {
        period = std::stoi(params["period"]);
    }

    const auto key = base32Decode(params["secret"]);
    uint64_t counter = now / period;

    int digits = 6;
    if (params.find("digits") != params.end()) {
        digits = std::stoi(params["digits"]);
    }

    return calculateTOTPInternal(algo, counter, digits, key);
}
