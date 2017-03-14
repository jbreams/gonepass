#pragma once

#include <functional>

#include <gtkmm.h>

using UnlockCallback = std::function<void(std::string path, std::string masterPassword)>;

class LockScreen : public Gtk::Box {
public:
    LockScreen(UnlockCallback _unlock_callback)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL),
          file_chooser(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER),
          password_field(),
          unlock_button("Unlock"),
          unlock_callback(_unlock_callback) {
        set_halign(Gtk::ALIGN_CENTER);
        set_valign(Gtk::ALIGN_CENTER);
        set_spacing(5);
        set_can_focus(false);

        auto welcome_label = Gtk::manage(new Gtk::Label("Unlock your vault"));
        pack_start(*welcome_label, false, true, 0);

        pack_start(file_chooser, false, true, 0);

        password_field.set_placeholder_text("Master Password");
        password_field.set_visibility(false);
        pack_start(password_field, false, true, 0);

        unlock_button.set_image_from_icon_name("dialog-password");
        unlock_button.set_always_show_image(true);
        pack_start(unlock_button, false, true, 0);

        unlock_button.signal_clicked().connect([this]() {
            auto password_text = password_field.get_text();
            auto vault_path = file_chooser.get_filename();

            unlock_callback(vault_path, password_text);
        });

        password_field.signal_activate().connect([this]() { unlock_button.clicked(); });

        show_all_children();
        password_field.grab_focus();
    };

    std::string getPassword() {
        return password_field.get_text();
    }

    void clearPassword() {
        password_field.set_text("");
    }

    std::string getPath() {
        return file_chooser.get_filename();
    }

    void setPath(std::string path) {
        file_chooser.set_filename(path);
    }

    virtual ~LockScreen(){};

protected:
    Gtk::FileChooserButton file_chooser;
    Gtk::Entry password_field;
    Gtk::Button unlock_button;

    UnlockCallback unlock_callback;
};
