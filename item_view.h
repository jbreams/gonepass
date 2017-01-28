#pragma once
#include <gtkmm.h>

#include "helper.h"
#include "keychain.h"

class ItemView : public Gtk::Grid {
public:
    ItemView(const KeychainItem& data) : Gtk::Grid() {
        set_row_spacing(5);
        set_column_spacing(3);
        set_margin_end(5);
        set_margin_start(5);

        if (data.sections.size() > 0) {
            for (const auto& section : data.sections) {
                attachSectionTitle(section.first);
                for (const auto& field : section.second) {
                    processSingleField(field);
                }
            }
        }

        if (!data.notes.empty()) {
            attachSectionTitle("Notes");
            auto notes_buffer = Gtk::TextBuffer::create();
            notes_buffer->set_text(data.notes);

            auto notes_field = Gtk::manage(new Gtk::TextView(notes_buffer));
            notes_field->set_hexpand(true);
            notes_field->set_vexpand(true);
            notes_field->set_editable(false);
            notes_field->set_wrap_mode(Gtk::WRAP_WORD);
            attach(*notes_field, 0, row_index++, 4, 1);
        }

        if (data.URLs.size() > 0) {
            attachSectionTitle("URLs");
            for (auto& url : data.URLs) {
                auto url_button = Gtk::manage(new Gtk::LinkButton(url, url));
                attach(*url_button, 0, row_index++, 4, 1);
            }
        }

        show_all_children();
    }
    virtual ~ItemView() {}

protected:
    void attachSectionTitle(std::string label) {
        auto label_widget = Gtk::manage(new Gtk::Label(label));
        attach(*label_widget, 0, row_index++, 4, 1);
    }

    void processSingleField(const KeychainField& field) {
        processSingleField(field.name, field.value, field.password);
    }

    void processSingleField(std::string label, std::string value, bool conceal) {
        auto my_index = row_index++;

        auto label_widget = Gtk::manage(new Gtk::Label(label));
        label_widget->set_halign(Gtk::ALIGN_END);
        label_widget->set_margin_end(5);
        attach(*label_widget, 0, my_index, 1, 1);

        auto value_widget = Gtk::manage(new Gtk::Entry());
        value_widget->set_hexpand(true);
        value_widget->set_editable(false);

        const auto isTOTP = isTOTPURI(value);
        if (isTOTP) {
            try {
                value_widget->set_text(calculateTOTP(value));
            } catch(std::exception& e) {
                errorDialog(e.what());
            }
        } else {
            value_widget->set_text(value);
        }

        if (conceal && !isTOTP)
            value_widget->set_visibility(false);
        attach(*value_widget, 1, my_index, conceal ? 1 : 3, 1);

        if (conceal) {
            auto copy_button = Gtk::manage(new Gtk::Button("_Copy", true));
            copy_button->signal_clicked().connect([value_widget]() {
                auto valueText = value_widget->get_text();
                auto clipboard = Gtk::Clipboard::get();
                clipboard->set_text(valueText);
                clipboard->store();
            });
            attach(*copy_button, 2, my_index, 1, 1);
            if (isTOTP) {
                auto calculate_button = Gtk::manage(new Gtk::Button("_Calculate", true));
                calculate_button->signal_clicked().connect([value, value_widget]() {
                    try {
                        value_widget->set_text(calculateTOTP(value));
                    } catch(std::exception& e) {
                        errorDialog(e.what());
                    }
                });
                attach(*calculate_button, 3, my_index, 1, 1);
            } else {
                auto reveal_button = Gtk::manage(new Gtk::Button("_Reveal", true));
                reveal_button->signal_clicked().connect([reveal_button, value_widget]() {
                    bool visible = value_widget->get_visibility();
                    visible = !visible;

                    value_widget->set_visibility(visible);
                    reveal_button->set_label(visible ? "_Hide" : "_Reveal");
                });
            }
        }
    }
    int row_index = 0;
};
