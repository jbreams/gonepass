#include <gtk/gtk.h>
#include "gonepass.h"

struct _GonepassApp {
    GtkApplication parent;
};

struct _GonepassAppClass {
    GtkApplicationClass parent_class;
};

G_DEFINE_TYPE(GonepassApp, gonepass_app, GTK_TYPE_APPLICATION);

static void gonepass_app_init(GonepassApp * app) {
    // Don't do anything yet!
}

static void quit_activated(GSimpleAction * action, GVariant * param, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

static void load_activated(GSimpleAction * action, GVariant * param, gpointer app) {
    GonepassAppWindow * win = gonepass_app_window_new(GONEPASS_APP(app));
    if(gonepass_app_window_credentials_loaded(win)) {
        gtk_application_add_window(app, GTK_WINDOW(win));
        gtk_window_present(GTK_WINDOW(win));
    }
    else {
        gtk_widget_destroy(GTK_WIDGET(win));
    }
}

static GActionEntry app_entries[] = {
    { "quit", quit_activated, NULL, NULL, NULL },
    { "load", load_activated, NULL, NULL, NULL }
};

static void gonepass_app_startup(GApplication * app) {
    GtkBuilder * builder;
    GMenuModel * app_menu;
    const char * quit_accels[2] = { "<Ctrl>Q", NULL };

    G_APPLICATION_CLASS(gonepass_app_parent_class)->startup(app);
    g_action_map_add_action_entries(G_ACTION_MAP(app),
        app_entries, G_N_ELEMENTS(app_entries), app);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", quit_accels);

    builder = gtk_builder_new_from_resource("/org/gtk/gonepass/appmenu.ui");
    app_menu = G_MENU_MODEL(gtk_builder_get_object(builder, "appmenu"));
    gtk_application_set_app_menu(GTK_APPLICATION(app), app_menu);
    g_object_unref(builder);
}

static void gonepass_app_activate(GApplication * app) {
    GonepassAppWindow * win = gonepass_app_window_new(GONEPASS_APP(app));
    if(gonepass_app_window_credentials_loaded(win))
        gtk_window_present(GTK_WINDOW(win));
    else
        gtk_widget_destroy(GTK_WIDGET(win));
}

static void gonepass_app_class_init(GonepassAppClass * class) {
    G_APPLICATION_CLASS(class)->startup = gonepass_app_startup;
    G_APPLICATION_CLASS(class)->activate = gonepass_app_activate;
}

GonepassApp * gonepass_app_new(void) {
    return g_object_new(GONEPASS_APP_TYPE,
        "application-id", "org.gtk.gonepassapp",
        "flags", G_APPLICATION_HANDLES_OPEN,
        NULL);
}
