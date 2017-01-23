#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"
// for convenience
using json = nlohmann::json;

struct KeychainField {
    std::string name;
    std::string value;
    std::string type;
    bool password;
};

struct KeychainItem {
    std::string title;
    std::string uuid;
    std::string category;
    std::string folder;
    std::unordered_map<std::string, std::vector<KeychainField>> sections;
    void addField(std::string section, KeychainField field) {
        auto it = sections.find(section);
        if (it == sections.end()) {
            sections.emplace(std::make_pair(section, std::vector<KeychainField>{field}));
        } else {
            it->second.push_back(std::move(field));
        }
    }
    std::vector<std::string> URLs;
    std::string website;
    std::string notes;
};

class AgileKeychainMasterKey {
public:
    AgileKeychainMasterKey(const json& input, const std::string masterPassword);

    json decryptItem(const json& input);
    json decryptJSON(const std::string& input);
    std::string encryptJSON(const json& input);

    std::string level;
    std::string id;

private:
    std::vector<uint8_t> key_data;
};

class Keychain {
public:
    Keychain(std::string path, std::string masterPassword);

    using ItemMap = std::unordered_map<std::string, KeychainItem>;
    ItemMap::iterator begin() {
        if (!loaded)
            reloadItems();
        return items.begin();
    }

    ItemMap::iterator end() {
        if (!loaded)
            reloadItems();
        return items.end();
    }

    ItemMap::iterator find(std::string key) {
        if (!loaded)
            reloadItems();
        return items.find(key);
    }

    void reloadItems();
    void unloadItems();

    std::string getTitle() {
        return title;
    }

    json decryptJSON(const std::string& input) {
        return level5_key->decryptJSON(input);
    }

    std::string encryptJSON(const json& input) {
        return level5_key->encryptJSON(input);
    }

private:
    void loadItem(std::string uuid);

    ItemMap items;
    std::unique_ptr<AgileKeychainMasterKey> level3_key, level5_key;
    std::string vault_path;
    std::string title;
    bool loaded = false;
};
