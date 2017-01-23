#pragma once
#include <gtkmm.h>
#include <memory>

#include "search_list.h"

class KeychainView : public Gtk::HPaned {
public:
    KeychainView(const std::shared_ptr<Keychain>& _keychain)
        : Gtk::HPaned(),
          keychain(_keychain),
          placeHolderWidget("Select an item...", Gtk::ALIGN_CENTER),
          scroller() {
        searchList = std::unique_ptr<SearchList>(new SearchList(
            [this](const Glib::ustring& uuid) { selectionChangedFn(uuid); }, *keychain));

        add1(*searchList);
        add2(scroller);
        scroller.add(placeHolderWidget);

        show_all_children();
    }
    virtual ~KeychainView(){};

protected:
    std::shared_ptr<Keychain> keychain;
    std::unique_ptr<SearchList> searchList = nullptr;
    void selectionChangedFn(const Glib::ustring& uuid) {
        auto newItemIter = keychain->find(uuid);
        // If this isn't in the keychain (weird!) return early
        if (newItemIter == keychain->end())
            return;

        const KeychainItem& newItem = newItemIter->second;
        cur_view = std::unique_ptr<ItemView>(new ItemView(newItem));
        scroller.remove_with_viewport();
        scroller.add(*cur_view);

        show_all_children();
    };

    Gtk::Label placeHolderWidget;
    Gtk::ScrolledWindow scroller;
    std::unique_ptr<ItemView> cur_view = nullptr;
};
