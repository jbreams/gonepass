#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <glibmm.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>

#include "keychain.h"
#include "evp_cipher.h"
#include "helper.h"

namespace {
using OpensslKeyData = std::pair<EVPKey, EVPIv>;
using SaltData = std::array<uint8_t, 8>;

OpensslKeyData opensslKey(const std::vector<uint8_t>& password, const SaltData& salt) {
    MD5_CTX ctx;
    EVPKey keyOut;
    EVPIv ivOut;
    keyOut.fill(0);
    ivOut.fill(0);

    // Initial digest is password + salt;
    MD5_Init(&ctx);
    MD5_Update(&ctx, password.data(), password.size());
    MD5_Update(&ctx, salt.data(), salt.size());
    MD5_Final(keyOut.data(), &ctx);

    MD5_Init(&ctx);
    MD5_Update(&ctx, keyOut.data(), 16);
    MD5_Update(&ctx, password.data(), password.size());
    MD5_Update(&ctx, salt.data(), salt.size());
    MD5_Final(ivOut.data(), &ctx);

    return OpensslKeyData(std::move(keyOut), std::move(ivOut));
}

OpensslKeyData opensslKeyNoSalt(const std::vector<uint8_t>& password) {
    MD5_CTX ctx;
    EVPKey keyOut;
    EVPIv ivOut;

    MD5_Init(&ctx);
    MD5_Update(&ctx, password.data(), password.size());
    MD5_Final(keyOut.data(), &ctx);

    return OpensslKeyData(keyOut, ivOut);
}

std::vector<uint8_t> base64Decode(const std::string& data) {
    std::vector<uint8_t> ret(data.size());

    auto b64 = BIO_new(BIO_f_base64());
    auto bmem = BIO_new_mem_buf(data.c_str(), data.size());
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
    int size = BIO_read(bmem, ret.data(), data.size());
    BIO_free_all(bmem);

    ret.resize(size);
    return ret;
}

std::string base64Encode(std::vector<uint8_t>& data) {
    auto b64 = BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem()));
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.data(), data.size());
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);

    std::string ret(bptr->data, bptr->length);

    BIO_free_all(b64);
    return ret;
}

template <typename T>
bool hasAllKeys(const json& d, T v) {
    return d.find(v) != d.end();
}

template <typename T, typename... Args>
bool hasAllKeys(const json& d, T first, Args... args) {
    return hasAllKeys(d, first) && hasAllKeys(d, args...);
}

std::array<uint8_t, 8> generateSalt() {
    static std::random_device engine;

    SaltData ret;
    int cur_index = 0;
    while (cur_index < ret.size()) {
        auto cur_random = engine();
        for (auto bits = (sizeof(cur_random) * 8) - 8; bits != 0; bits -= 8) {
            uint8_t byte = (cur_random >> bits) & 0xff;
            ret[cur_index++] = byte;
        }
    }

    return ret;
}
}

using RawKeyData = std::tuple<std::array<uint8_t, 8>, std::vector<uint8_t>, bool>;
RawKeyData parseEncryptedString(const std::string& data) {
    auto raw_key_data = base64Decode(data);
    std::array<uint8_t, 8> salt;

    if (raw_key_data.size() < 8) {
        throw std::runtime_error("Master key data is too short");
    }

    std::string saltCheck(raw_key_data.begin(), raw_key_data.begin() + 8);
    if (saltCheck == "Salted__") {
        std::vector<uint8_t> output_data;
        std::copy_n(raw_key_data.begin() + 8, 8, salt.begin());
        std::copy(raw_key_data.begin() + 16, raw_key_data.end(), std::back_inserter(output_data));
        return RawKeyData(salt, output_data, true);
    }
    return RawKeyData(salt, raw_key_data, false);
}

AgileKeychainMasterKey::AgileKeychainMasterKey(const json& input,
                                               const std::string masterPassword) {
    if (!hasAllKeys(input, "data", "iterations", "validation", "level", "identifier"))
        throw std::runtime_error("Master key data does not have required fields");

    RawKeyData input_key_data = parseEncryptedString(input["data"]);
    std::array<uint8_t, 32> master_key;

    // Generate a 32-byte key from the master password and its salt
    PKCS5_PBKDF2_HMAC_SHA1(masterPassword.c_str(),
                           masterPassword.size(),
                           std::get<0>(input_key_data).data(),
                           std::get<0>(input_key_data).size(),
                           input["iterations"],
                           master_key.size(),
                           master_key.data());

    // This block decrypts the master key using the master password and stores it
    // in ret. If the
    // master key cannot be decrypted, it raises an error.
    {
        try {
            EVPKey master_aes_key;
            EVPIv master_aes_iv;
            std::copy_n(master_key.begin(), 16, master_aes_key.begin());
            std::copy(master_key.begin() + 16, master_key.end(), master_aes_iv.begin());

            EVPCipher cipher(EVP_aes_128_cbc(), master_aes_key, master_aes_iv, false);
            cipher.update(std::get<1>(input_key_data));
            key_data = std::vector<uint8_t>(cipher.cbegin(), cipher.cend());
        } catch (EVPCipherException& e) {
            throw std::runtime_error("Couldn't decrypt master key!");
        }
    }

    OpensslKeyData validation_keys;
    auto validation_data = parseEncryptedString(input["validation"]);
    if (std::get<2>(validation_data)) {
        validation_keys = opensslKey(key_data, std::get<0>(validation_data));
    } else {
        validation_keys = opensslKeyNoSalt(key_data);
    }

    // This block decrypts the validation key and validates the master key with it
    {
        try {
            EVPCipher cipher(EVP_aes_128_cbc(),
                             std::get<0>(validation_keys),
                             std::get<1>(validation_keys),
                             false);
            cipher.update(std::get<1>(validation_data));
            cipher.finalize();
            if (cipher.accumulator != key_data) {
                throw std::runtime_error("Couldn't verify master key!");
            }
        } catch (EVPCipherException& e) {
            throw std::runtime_error("Couldn't decrypt validation_key!");
        }
    }

    level = input["level"];
    id = input["identifier"];
}

json AgileKeychainMasterKey::decryptItem(const json& input) {
    return decryptJSON(input["encrypted"]);
}

std::string AgileKeychainMasterKey::encryptJSON(const json& input) {
    const auto payload_str = input.dump();
    const auto new_salt = generateSalt();
    const auto cipher_keys = opensslKey(key_data, new_salt);

    std::vector<uint8_t> encrypted_payload;
    encrypted_payload.reserve(1024);
    const std::string salted_str = "Salted__";
    std::copy(salted_str.begin(), salted_str.end(), std::back_inserter(encrypted_payload));
    std::copy(new_salt.begin(), new_salt.end(), std::back_inserter(encrypted_payload));
    try {
        EVPCipher cipher(
            EVP_aes_128_cbc(), std::get<0>(cipher_keys), std::get<1>(cipher_keys), true);

        cipher.update(payload_str);
        std::copy(cipher.begin(), cipher.end(), std::back_inserter(encrypted_payload));

    } catch (EVPCipherException& e) {
        throw std::runtime_error("Couldn't encrypt item");
    }

    return base64Encode(encrypted_payload);
}

json AgileKeychainMasterKey::decryptJSON(const std::string& input) {
    auto raw_payload = parseEncryptedString(input);
    OpensslKeyData cipher_keys;
    if (std::get<2>(raw_payload)) {
        cipher_keys = opensslKey(key_data, std::get<0>(raw_payload));
    } else {
        cipher_keys = opensslKeyNoSalt(key_data);
    }

    try {
        EVPCipher cipher(
            EVP_aes_128_cbc(), std::get<0>(cipher_keys), std::get<1>(cipher_keys), false);

        cipher.update(std::get<1>(raw_payload));

        const std::string json_str(cipher.cbegin(), cipher.cend());
        return json::parse(json_str);
    } catch (EVPCipherException& e) {
        throw std::runtime_error("Couldn't decrypt item");
    }
}

Keychain::Keychain(std::string path, std::string masterPassword) {
    json keys_json;
    // Load the keys file into a json object
    {
        std::stringstream keys_path;
        keys_path << path << "/data/default/encryptionKeys.js";
        auto encryption_keys_fp = std::ifstream(keys_path.str());
        if (!encryption_keys_fp)
            throw std::runtime_error("Error loading encryption keys file");
        encryption_keys_fp >> keys_json;
    }

    auto list = keys_json["list"];
    if (!list.is_array()) {
        throw std::runtime_error("Could not find list of keys in keychain");
    }

    for (const auto key : list) {
        std::unique_ptr<AgileKeychainMasterKey> cur_master_key(
            new AgileKeychainMasterKey(key, masterPassword));
        if (cur_master_key->level == "SL3")
            level3_key = std::move(cur_master_key);
        else if (cur_master_key->level == "SL5")
            level5_key = std::move(cur_master_key);
        else {
            throw std::runtime_error("Unknown security level for master key");
        }
    }

    vault_path = path;
}

void Keychain::loadItem(std::string uuid) {
    KeychainItem item;
    json item_json;

    {
        std::stringstream item_path;
        item_path << vault_path << "/data/default/" << uuid << ".1password";
        auto item_fp = std::ifstream(item_path.str());
        if (!item_fp) {
            throw std::runtime_error("Cannot load item file");
        }
        item_fp >> item_json;
    }

    auto typeName = item_json["typeName"];
    std::string securityLevel;
    if (item_json.find("securityLevel") == item_json.end()) {
        auto openContents = item_json.find("openContents");
        if (openContents == item_json.end())
            throw std::runtime_error("Could not find security level for item");
        securityLevel = (*openContents)["securityLevel"];
    } else {
        if (item_json.find("securityLevel") == item_json.end())
            throw std::runtime_error("Could not find security level for item");
        securityLevel = item_json["securityLevel"];
    }

    json decrypted_item;
    if (securityLevel == "SL5")
        decrypted_item = level5_key->decryptItem(item_json);
    else if (securityLevel == "SL3")
        decrypted_item = level3_key->decryptItem(item_json);
    else
        throw std::runtime_error("Invalid security level for item");

    item.title = item_json["title"];
    item.uuid = uuid;

    if (decrypted_item.find("notesPlain") != decrypted_item.end())
        item.notes = decrypted_item["notesPlain"];

    if (decrypted_item.find("URLs") != decrypted_item.end()) {
        for (auto url_obj : decrypted_item["URLs"]) {
            item.URLs.emplace_back(url_obj["url"].get<std::string>());
        }
    }

    if (decrypted_item.find("password") != decrypted_item.end()) {
        std::string value_str = decrypted_item["password"];
        item.addField("", {"password", value_str, "P", true});
    }

    auto fields = decrypted_item.find("fields");
    if (fields != decrypted_item.end()) {
        for (const auto& field : *fields) {
            if (!hasAllKeys(field, "designation", "value", "type"))
                continue;
            auto typeStr = field["type"];
            bool isPassword = typeStr == "P";
            item.addField("", {field["designation"], field["value"], typeStr, isPassword});
        }
    }

    auto sections = decrypted_item.find("sections");
    if (sections != decrypted_item.end()) {
        for (const auto& section : *sections) {
            auto section_fields = section.find("fields");
            if (section_fields == section.end()) {
                continue;
            }

            std::string section_title;
            if (section.find("title") != section.end())
                section_title = section["title"];

            for (const auto& field : *section_fields) {
                if (!hasAllKeys(field, "k", "t", "v"))
                    continue;
                std::string typeStr = field["k"];
                std::string nameStr = field["t"];
                auto value = field["v"];
                std::string valueStr;
                bool isPassword = typeStr == "concealed";

                if (typeStr == "date") {
                    char dateBuf[20] = {'\0'};
                    time_t time_val = static_cast<time_t>(value);
                    std::strftime(dateBuf, sizeof(dateBuf), "%x", std::localtime(&time_val));
                    valueStr = dateBuf;
                } else if (typeStr == "address") {
                    std::stringstream ss;
                    if (!value["street"].is_string())
                        continue;

                    ss << value["street"].get<std::string>() << " "
                       << value["city"].get<std::string>() << ", "
                       << value["state"].get<std::string>() << " "
                       << value["zip"].get<std::string>();
                    valueStr = ss.str();
                } else if (typeStr == "monthYear") {
                    auto stringVal = std::to_string(value.get<int>());
                    std::stringstream ss;
                    ss << stringVal.substr(0, 4) << "/" << stringVal.substr(4, 2);
                    valueStr = ss.str();
                } else {
                    valueStr = value;
                }

                item.addField(section_title, {nameStr, valueStr, typeStr, isPassword});
            }
        }
    }

    items.insert({uuid, std::move(item)});
}

void Keychain::reloadItems() {
    items.clear();
    json contents_json;
    {
        std::stringstream contents_path;
        contents_path << vault_path << "/data/default/contents.js";
        auto contents_fp = std::ifstream(contents_path.str());
        if (!contents_fp)
            throw std::runtime_error("Cannot open keychain contents");
        contents_json << contents_fp;
    }

    for (const auto& contents_item : contents_json) {
        try {
            loadItem(contents_item[0]);
        } catch (std::exception& e) {
            std::stringstream ss;
            ss << "Error loading item " << contents_item[2] << ": " << e.what();
            errorDialog(ss.str());
        }
    }

    loaded = true;
}

void Keychain::unloadItems() {
    items.clear();
    loaded = false;
}
