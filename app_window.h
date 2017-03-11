#pragma once
#include <functional>
#include <set>

#include <gtkmm.h>

#include "app_menu.h"
#include "config_storage.h"
#include "keychain_container.h"

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow() : Gtk::ApplicationWindow() {
        std::initializer_list<CallbackData> actions = {
            {"_Lock Vaults", [this]() { lockVaults(); }},
            {"_Load New Vault", [this]() { addNewVault(); }},
            {"_Refresh Vaults", [this]() { refreshVaults(); }},
            {"_Quit", [this]() { get_application()->quit(); }}};

        set_default_geometry(800, 480);

        app_menu = std::unique_ptr<AppMenu>(new AppMenu(actions));

        set_titlebar(header_bar);
        header_bar.set_title("Gonepass");
        header_bar.set_has_subtitle(true);
        header_bar.set_show_close_button(false);
        header_bar.pack_start(*app_menu);

        if (config_cache.find("loaded_vaults") != config_cache.end()) {
            auto loaded_vaults = config_cache["loaded_vaults"];
            for (auto it = loaded_vaults.begin(); it != loaded_vaults.end(); ++it) {
                addCachedVault(it.key(), false);
            }
        }
        if (config_cache.find("master_vault") != config_cache.end()) {
            addCachedVault(config_cache["master_vault"], true);
        } else {
            addNewVault();
        }
        show_all_children();
    };

    virtual ~MainWindow(){};

protected:
    void addNewVault() {
        remove();
        auto new_vault = std::make_shared<KeychainContainer>("");
        auto unlock_cb = [new_vault, this](std::string title, std::string password) {
            unlockCb(new_vault, title, password);
        };
        new_vault->setUnlockCb(unlock_cb);
        add(*new_vault);
        show_all_children();
    }

    void unlockCb(std::shared_ptr<KeychainContainer> new_vault,
                  std::string title,
                  std::string password) {
        auto app_menu_select_cb = [new_vault, this]() {
            remove();
            add(*new_vault);
            header_bar.set_subtitle(new_vault->getPath());
            show_all_children();
        };

        if (!master_vault) {
            master_vault = new_vault;
        }
        if (master_vault == new_vault) {
            unlockAllVaults();
        }
        if (master_vault->getKeychain())
            updateCache(title, password);

        if (container_list.find(new_vault) == container_list.end()) {
            app_menu->addVault(title, app_menu_select_cb);
            container_list.emplace(new_vault);
        }

        header_bar.set_subtitle(title);
        show_all_children();
    }

    void addCachedVault(std::string path, bool master) {
        auto new_vault = std::make_shared<KeychainContainer>(path);
        auto unlock_cb = [new_vault, this](std::string title, std::string password) {
            unlockCb(new_vault, title, password);
        };
        new_vault->setUnlockCb(unlock_cb);
        auto app_menu_select_cb = [new_vault, this]() {
            remove();
            add(*new_vault);
            header_bar.set_subtitle(new_vault->getPath());
            show_all_children();
        };

        container_list.emplace(new_vault);
        if (master) {
            std::stringstream title_builder;
            title_builder << path << " (master vault)";
            app_menu->prependVault(title_builder.str(), app_menu_select_cb);
            master_vault = new_vault;
        } else
            app_menu->addVault(path, app_menu_select_cb);
        app_menu_select_cb();
    }

    void lockVaults() {
        for (auto&& keychain_view : container_list) {
            keychain_view->lock();
        }
    }

    void refreshVaults() {
        for (auto&& keychain_view : container_list) {
            keychain_view->refresh();
        }
    }

    void updateCache(std::string path, std::string password) {
        if (config_cache.find("master_vault") == config_cache.end()) {
            config_cache["master_vault"] = master_vault->getPath();
            config_cache.save();
            return;
        } else if (path == config_cache["master_vault"])
            return;

        json cached_data_dict;
        if (config_cache.find("loaded_vaults") != config_cache.end()) {
            cached_data_dict = config_cache["loaded_vaults"];
        }

        json cached_data;
        cached_data["password"] = password;
        cached_data_dict[path] = master_vault->getKeychain()->encryptJSON(cached_data);
        config_cache["loaded_vaults"] = cached_data_dict;
        config_cache.save();
    }

    void unlockAllVaults() {
        if (!master_vault) {
            return;
        }

        if (config_cache.find("loaded_vaults") == config_cache.end()) {
            return;
        }
        auto cached_vaults = config_cache["loaded_vaults"];

        for (auto vault : container_list) {
            if (vault == master_vault) {
                continue;
            }

            auto cur_vault_data = cached_vaults.find(vault->getPath());
            if (cur_vault_data == cached_vaults.end()) {
                continue;
            }
            auto decrypted_data = master_vault->getKeychain()->decryptJSON(*cur_vault_data);
            std::string decrypted_password = decrypted_data["password"];
            vault->unlock(decrypted_password);
        }
    }

    ConfigCache config_cache;
    std::shared_ptr<KeychainContainer> master_vault;
    std::unique_ptr<AppMenu> app_menu;
    std::set<std::shared_ptr<KeychainContainer>> container_list;
    Gtk::HeaderBar header_bar;
};
