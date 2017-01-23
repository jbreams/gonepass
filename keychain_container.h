#pragma once

#include <gtkmm.h>

#include "helper.h"
#include "keychain_view.h"
#include "lock_screen.h"

class KeychainContainer : public Gtk::Bin {
public:
    KeychainContainer(std::string path) : Gtk::Bin() {
        lock_screen = std::unique_ptr<LockScreen>(
            new LockScreen([this](std::string path, std::string master_password) {
                unlock_callback(path, master_password);
            }));
        if (!path.empty()) {
            lock_screen->setPath(path);
        }
        add(*lock_screen);
        show_all_children();
    }

    void refresh() {
        keychain_object->reloadItems();
        keychain_view = std::unique_ptr<KeychainView>(new KeychainView(keychain_object));
        remove();
        add(*keychain_view);
        show_all_children();
    }

    void lock() {
        remove();
        keychain_object.reset();
        keychain_view.reset();
        add(*lock_screen);
        show_all_children();
    }

    void setUnlockCb(std::function<void(std::string title, std::string password)> fn) {
        parent_unlock_cb = fn;
    }

    std::shared_ptr<Keychain> getKeychain() {
        return std::shared_ptr<Keychain>(keychain_object);
    }

    void unlock(std::string master_password) {
        unlock_impl(getPath(), master_password);
    }

    std::string getPath() {
        return lock_screen->getPath();
    }

protected:
    void unlock_impl(std::string path, std::string master_password) {
        try {
            keychain_object = std::make_shared<Keychain>(path, master_password);
        } catch (std::exception& e) {
            errorDialog(e.what());
            return;
        }
        remove();
        keychain_view = std::unique_ptr<KeychainView>(new KeychainView(keychain_object));
        add(*keychain_view);
        show_all_children();
    }
    void unlock_callback(std::string path, std::string master_password) {
        unlock_impl(path, master_password);
        parent_unlock_cb(path, lock_screen->getPassword());
        lock_screen->clearPassword();
    }

    std::unique_ptr<LockScreen> lock_screen;
    std::shared_ptr<Keychain> keychain_object;
    std::unique_ptr<KeychainView> keychain_view;
    std::function<void(std::string title, std::string password)> parent_unlock_cb;
};
