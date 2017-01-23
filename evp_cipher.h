#pragma once
#include <array>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <vector>

using EVPKey = std::array<uint8_t, EVP_MAX_KEY_LENGTH>;
using EVPIv = std::array<uint8_t, EVP_MAX_IV_LENGTH>;

class EVPCipherException : public std::runtime_error {
public:
    EVPCipherException()
        : std::runtime_error("EVP cipher error"), err_code{ERR_peek_last_error()}, err_msg_buf{0} {}

    virtual const char* what() {
        return ERR_error_string(err_code, err_msg_buf);
    }

    unsigned long err_code;
    char err_msg_buf[128];
};

class EVPCipher {
private:
    std::function<void(EVP_CIPHER_CTX*)> cipherFreeFn = [](EVP_CIPHER_CTX* ptr) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        delete ptr;
#else
        EVP_CIPHER_CTX_free(ptr);
#endif
    };

public:
    EVPCipher(const EVP_CIPHER* type, const EVPKey& key, const EVPIv& iv, bool encrypt) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        ctx = std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)>(new EVP_CIPHER_CTX,
                                                                      cipherFreeFn);
#else
        ctx = std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)>(EVP_CIPHER_CTX_new(),
                                                                      cipherFreeFn);
#endif
        EVP_CIPHER_CTX_init(ctx.get());
        EVP_CipherInit_ex(ctx.get(), type, nullptr, key.data(), iv.data(), encrypt ? 1 : 0);
    }

    ~EVPCipher() {
        EVP_CIPHER_CTX_cleanup(ctx.get());
    }

    void expandAccumulator(size_t inputSize) {
        auto block_size = EVP_CIPHER_CTX_block_size(ctx.get());
        const auto maxIncrease = (((inputSize / block_size) + 1) * block_size);
        accumulator.resize(accumulator.size() + maxIncrease);
    }

    void update(const std::vector<uint8_t>& data) {
        const auto oldSize = accumulator.size();
        expandAccumulator(data.size());
        int encryptedSize = 0;
        if (EVP_CipherUpdate(ctx.get(),
                             accumulator.data() + oldSize,
                             &encryptedSize,
                             data.data(),
                             data.size()) != 1) {
            throw EVPCipherException();
        }

        accumulator.resize(oldSize + encryptedSize);
    }

    void update(const std::string& data) {
        const auto oldSize = accumulator.size();
        expandAccumulator(data.size());
        int encryptedSize = 0;

        auto data_ptr = reinterpret_cast<const uint8_t*>(data.data());
        if (EVP_CipherUpdate(
                ctx.get(), accumulator.data() + oldSize, &encryptedSize, data_ptr, data.size()) !=
            1) {
            throw EVPCipherException();
        }

        accumulator.resize(oldSize + encryptedSize);
    }

    void finalize() {
        if (finalized)
            return;
        const auto oldSize = accumulator.size();
        // Add one more block size to the accumulator;
        expandAccumulator(1);
        int encryptSize = oldSize;
        if (EVP_CipherFinal_ex(ctx.get(), accumulator.data() + oldSize, &encryptSize) != 1) {
            throw EVPCipherException();
        }
        accumulator.resize(oldSize + encryptSize);
        finalized = true;
    }

    std::vector<uint8_t>::iterator begin() {
        finalize();
        return accumulator.begin();
    }

    std::vector<uint8_t>::iterator end() {
        finalize();
        return accumulator.end();
    }

    std::vector<uint8_t>::const_iterator cbegin() {
        finalize();
        return accumulator.cbegin();
    }

    std::vector<uint8_t>::const_iterator cend() {
        finalize();
        return accumulator.cend();
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)> ctx;
    std::vector<uint8_t> accumulator;
    bool finalized = false;
};
