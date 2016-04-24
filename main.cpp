#include "item_view.h"
#include "search_list.h"
#include "keychain_view.h"
#include "app_window.h"

int main(int argc, char** argv) {
    auto app = Gtk::Application::create(argc, argv);
    MainWindow window;
    return app->run(window);
}
