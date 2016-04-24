#pragma once

#include <gtkmm.h>

#include "keychain_container.h"

struct CallbackData {
    std::string label;
    std::function<void()> callback;
};

class AppMenu : public Gtk::MenuButton {
public:
    AppMenu(std::initializer_list<CallbackData> action_callbacks) : Gtk::MenuButton() {
        auto menu_button_pixbuf =
            Gdk::Pixbuf::create_from_resource("/org/gtk/gonepass/gonepass-icon.png", 36, -1);
        auto menu_button_icon = Gtk::manage(new Gtk::Image(menu_button_pixbuf));
        set_image(*menu_button_icon);
        set_menu(menu);

        for (auto&& item_def : action_callbacks) {
            auto new_item = Gtk::manage(new Gtk::MenuItem(item_def.label, true));
            new_item->signal_activate().connect(item_def.callback);
            menu.append(*new_item);
            new_item->show();
            vault_items_start++;
        }

        auto seperator_item = Gtk::manage(new Gtk::SeparatorMenuItem());
        menu.append(*seperator_item);
        seperator_item->show();
        vault_items_start;
    }

    void addVault(std::string title, std::function<void()> activateCallback) {
        auto new_menu_item = Gtk::manage(new Gtk::RadioMenuItem(menu_group, title));
        new_menu_item->signal_activate().connect(activateCallback);
        menu.append(*new_menu_item);
        new_menu_item->show();
        new_menu_item->set_active(true);
    }

    void prependVault(std::string title, std::function<void()> activateCallback) {
        auto new_menu_item = Gtk::manage(new Gtk::RadioMenuItem(menu_group, title));
        new_menu_item->signal_activate().connect(activateCallback);
        menu.insert(*new_menu_item, vault_items_start);
        new_menu_item->show();
        new_menu_item->set_active(true);
    }

private:
    Gtk::Menu menu;
    int vault_items_start = 1;
    Gtk::RadioMenuItem::Group menu_group;
};
