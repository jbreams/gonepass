#pragma once
#include <functional>
#include <gtkmm.h>

#include "keychain.h"

class SearchList : public Gtk::VBox {
public:
    SearchList(std::function<void(const Glib::ustring&)> _selectionChangedCb, Keychain& keychain)
        : Gtk::VBox() {
        selectionChangedCb = _selectionChangedCb;
        set_spacing(5);
        set_can_focus(false);

        pack_start(search_entry, false, true, 0);
        pack_start(viewport, true, true, 0);

        viewport.add(item_list);
        viewport.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

        item_list_model = Gtk::ListStore::create(columns);

        for (const auto& item : keychain) {
            auto uuid = item.first;
            auto title = item.second.title;

            auto new_row = *item_list_model->append();
            new_row[columns.uuid] = uuid;
            new_row[columns.name] = title;
        }

        item_list_model->set_sort_column(columns.name, Gtk::SORT_ASCENDING);

        item_list_filter_model = Gtk::TreeModelFilter::create(item_list_model);
        item_list_filter_model->set_visible_func(
            [this](const Gtk::TreeModel::const_iterator& iter) -> bool {
                if (!iter)
                    return true;
                auto row = *iter;
                Glib::ustring itemName = row[columns.name];
                auto searchText = search_entry.get_text();

                if (itemName.casefold().find(searchText.casefold()) != Glib::ustring::npos)
                    return true;
                return false;
            });

        item_list.set_model(item_list_filter_model);
        item_list.append_column("Name", columns.name);
        item_list.set_headers_visible(false);

        item_list_selector = item_list.get_selection();
        item_list_selector->set_mode(Gtk::SELECTION_BROWSE);
        item_list_selector->signal_changed().connect([this]() {
            auto row = *item_list.get_selection()->get_selected();
            auto itemUUID = row[columns.uuid];

            selectionChangedCb(itemUUID);
        });

        search_entry.signal_search_changed().connect(
            [this]() { item_list_filter_model->refilter(); });
    };

    virtual ~SearchList(){};

protected:
    class SearchListColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        SearchListColumns() {
            add(uuid);
            add(name);
        }

        Gtk::TreeModelColumn<Glib::ustring> uuid;
        Gtk::TreeModelColumn<Glib::ustring> name;
    };

    SearchListColumns columns;

    Gtk::ScrolledWindow viewport;
    Gtk::SearchEntry search_entry;
    Gtk::TreeView item_list;
    Glib::RefPtr<Gtk::TreeSelection> item_list_selector;
    Glib::RefPtr<Gtk::ListStore> item_list_model;
    Glib::RefPtr<Gtk::TreeModelFilter> item_list_filter_model;
    std::function<void(const Glib::ustring&)> selectionChangedCb;
};
