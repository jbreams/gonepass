#include "app_window.h"
#include "item_view.h"
#include "keychain_view.h"
#include "search_list.h"

#include <openssl/evp.h>

MainWindow* getMainWindow() {
    static MainWindow win;
    return &win;
}

void errorDialog(const std::string& msg) {
    Gtk::MessageDialog dlg(*getMainWindow(), msg);
    dlg.run();
}

int main(int argc, char** argv) {
    OpenSSL_add_all_algorithms();
    auto app = Gtk::Application::create(argc, argv);
    auto mainWindow = getMainWindow();
    return app->run(*mainWindow);
}
