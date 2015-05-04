#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <jansson.h>
#include <string.h>
#include <strings.h>
#include "gonepass.h"

struct _GonepassAppWindow {
    GtkApplicationWindow parent;
};

struct _GonepassAppWindowClass {
    GtkApplicationWindowClass parent_class;
};

typedef struct _GonepassAppWindowPrivate GonepassAppWindowPrivate;
struct _GonepassAppWindowPrivate {
    GSettings * settings;
    GtkWidget * item_list_box;
    GtkSearchEntry * item_search;
    GtkTreeView * item_list;
    GtkListStore * item_list_store;
    GtkTreeModel * item_list_sorter;
    GtkTreeModel * item_list_filterer;
    GtkLabel * item_name;
    GtkBox * entries_container;

    GtkBox * locked_container;
    GtkPaned * unlocked_container;

    GtkEntry * master_password_field;
    GtkButton * unlock_vault_button;
    GtkFileChooserButton * vault_chooser_button;

    struct credentials_bag bag;
};

G_DEFINE_TYPE_WITH_PRIVATE(GonepassAppWindow, gonepass_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void update_item_list(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    char vault_path[PATH_MAX];
    snprintf(vault_path, PATH_MAX, "%s/data/default/contents.js", priv->bag.vault_path);
    json_error_t json_err;
    json_t * contents_js = json_load_file(vault_path, JSON_ALLOW_NUL, &json_err);
    if(contents_js == NULL) {

        errmsg_box_win(win, "Error loading contents.js: %s", json_err.text);
        return;
    }

    gtk_list_store_clear(priv->item_list_store);

    size_t contents_index;
    json_t * cur_item;
    GtkListStore * model = priv->item_list_store;

    json_array_foreach(contents_js, contents_index, cur_item) {
        char * name, *uuid, *type;
        GtkTreeIter iter;
        json_unpack(cur_item, "[s s s]", &name, &type, &uuid);

        sprintf(vault_path, "%s/data/default/%s.1password", priv->bag.vault_path, name);

        json_t * item_json = json_load_file(vault_path, JSON_ALLOW_NUL, &json_err);
        if(item_json == NULL) {
            errmsg_box_win(win, "Cannot load item file! %s\n", json_err.text);
            continue;
        }

        char * decrypted_payload;
        int payload_size = decrypt_item(item_json, &priv->bag, &decrypted_payload);

        if(payload_size > 0)
            free(decrypted_payload);
        json_decref(item_json);

        // This is the smallest possible JSON object I could think of.
        if(payload_size < 8)
            continue;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
            0, name,
            1, uuid,
            -1
        );
    }
}

static gboolean item_list_filter_viewable(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    GonepassAppWindow * win = GONEPASS_APP_WINDOW(data);
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    gchar * cur_filtering;
    gtk_tree_model_get(model, iter, 1, &cur_filtering, -1);
    const gchar * cur_searched = gtk_entry_get_text(GTK_ENTRY(priv->item_search));

    if(!cur_filtering)
        return FALSE;
    if(strcasestr(cur_filtering, cur_searched) == NULL)
        return FALSE;
    return TRUE;
}

static void item_search_changed(GtkSearchEntry * entry, gpointer data) {
    GonepassAppWindow * win = GONEPASS_APP_WINDOW(data);
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(priv->item_list_filterer));
}

static void clear_entries_container_cb(GtkWidget * child, gpointer parent) {
    gtk_container_remove(GTK_CONTAINER(parent), child);
}

void item_list_selection_changed_cb(GtkTreeSelection * selection, GonepassAppWindow * win) {
    GtkTreeIter iter;
    GonepassAppWindowPrivate *priv = gonepass_app_window_get_instance_private(win);
    GtkTreeModel * model;
    gchar * uuid, *name;
    char item_file_path[PATH_MAX], *encrypted_text, *security_level;
    json_error_t json_err;

    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &uuid, -1);
    gtk_tree_model_get(model, &iter, 1, &name, -1);

    sprintf(item_file_path, "%s/data/default/%s.1password", priv->bag.vault_path, uuid);

    char namebuf[1024];
    snprintf(namebuf, sizeof(namebuf) - 1, "<span size=\"x-large\">%s</span>", name);
    gtk_label_set_markup(priv->item_name, namebuf);

    gtk_container_foreach(GTK_CONTAINER(priv->entries_container),
        clear_entries_container_cb, priv->entries_container);

    json_t * item_json = json_load_file(item_file_path, JSON_ALLOW_NUL, &json_err);
    if(item_json == NULL) {
        errmsg_box_win(win, "Cannot load item file! %s\n", json_err.text);
        return;
    }
    g_free(uuid);

    char * decrypted_payload;
    int payload_size = decrypt_item(item_json, &priv->bag, &decrypted_payload);
    if(payload_size == -1)
        return;
    json_decref(item_json);

    json_t * decrypted_item = json_loadb(decrypted_payload, payload_size, JSON_ALLOW_NUL, &json_err);
    free(decrypted_payload);
    if(decrypted_item == NULL) {
        errmsg_box_win(win, "Error loading decrypted JSON: %s\n", json_err.text);
        return;
    }

    process_entries(decrypted_item, GTK_WIDGET(priv->entries_container));
    gtk_widget_show_all(GTK_WIDGET(priv->entries_container));
    json_decref(decrypted_item);
}

static void unlock_button_cb(GtkButton * button, gpointer user) {
    GonepassAppWindow * win = GONEPASS_APP_WINDOW(user);
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);

    const gchar * master_pass = gtk_entry_get_text(priv->master_password_field);
    const gchar * vault_path = gtk_file_chooser_get_filename(
        GTK_FILE_CHOOSER(priv->vault_chooser_button));

    if(load_credentials(win, master_pass, vault_path, &priv->bag) == -1)
        return;
    gtk_window_set_title(GTK_WINDOW(win), priv->bag.vault_path);
    update_item_list(win);

    gtk_widget_set_visible(GTK_WIDGET(priv->locked_container), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(priv->unlocked_container), TRUE);
    g_settings_set_string(priv->settings, "vault-path", vault_path);
}

int gonepass_app_window_credentials_loaded(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    return priv->bag.credentials_loaded;
}

static void gonepass_app_window_init(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv;
    GtkBuilder * builder;
    GAction * action;
    GtkCellRenderer * renderer;
    GtkTreeViewColumn * column;
    GtkTreeSelection * selector;

    priv = gonepass_app_window_get_instance_private(win);
    gtk_widget_init_template(GTK_WIDGET(win));
    priv->settings = g_settings_new("org.gtk.gonepass");
    priv->item_list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    priv->item_list_sorter = gtk_tree_model_sort_new_with_model(
        GTK_TREE_MODEL(priv->item_list_store));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->item_list_sorter),
        1, GTK_SORT_ASCENDING);

    priv->item_list_filterer = gtk_tree_model_filter_new(priv->item_list_sorter, NULL);
    gtk_tree_model_filter_set_visible_func(
        GTK_TREE_MODEL_FILTER(priv->item_list_filterer), item_list_filter_viewable, win, NULL);

    gtk_tree_view_set_model(priv->item_list, GTK_TREE_MODEL(priv->item_list_filterer));
    selector = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->item_list));
    gtk_tree_selection_set_mode(selector, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(selector), "changed", G_CALLBACK(item_list_selection_changed_cb), win);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
            "Name",
            renderer,
            "text", 1,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->item_list), column);

    g_signal_connect(G_OBJECT(priv->item_search), "search-changed", G_CALLBACK(item_search_changed), win);
    g_signal_connect(G_OBJECT(priv->unlock_vault_button), "clicked", G_CALLBACK(unlock_button_cb), win);

    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->vault_chooser_button),
        g_settings_get_string(priv->settings, "vault-path"));
    gtk_window_set_default(GTK_WINDOW(win), GTK_WIDGET(priv->unlock_vault_button));
}

static void gonepass_app_window_dispose(GObject * object) {
    GonepassAppWindow * win = GONEPASS_APP_WINDOW(object);
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);

    g_clear_object(&priv->settings);
    G_OBJECT_CLASS(gonepass_app_window_parent_class)->dispose(object);
}

static void gonepass_app_window_class_init(GonepassAppWindowClass * class) {
    G_OBJECT_CLASS(class)->dispose = gonepass_app_window_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
        "/org/gtk/gonepass/gonepass.glade");

    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, item_search);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, item_list);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, entries_container);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, item_name);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, vault_chooser_button);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, master_password_field);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, unlock_vault_button);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, locked_container);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, unlocked_container);
}

GonepassAppWindow * gonepass_app_window_new(GonepassApp * app) {
    GonepassAppWindow * win = g_object_new(GONEPASS_APP_WINDOW_TYPE, "application", app, NULL);
    return win;
}

void gonepass_app_window_open(GonepassAppWindow * win, GFile * file) {
    GFileType target_type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    if(target_type != G_FILE_TYPE_DIRECTORY)
        return;

    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    gtk_file_chooser_set_filename(
        GTK_FILE_CHOOSER(priv->vault_chooser_button),
        g_file_get_path(file)
    );
}

void gonepass_app_window_lock(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    gtk_entry_set_text(GTK_ENTRY(priv->master_password_field), "");

    gtk_widget_set_visible(GTK_WIDGET(priv->unlocked_container), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(priv->locked_container), TRUE);

    gtk_container_foreach(GTK_CONTAINER(priv->entries_container),
        clear_entries_container_cb, priv->entries_container);
    memset(&priv->bag, 0, sizeof(priv->bag));
}

void gonepass_app_window_refresh(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    if(priv->bag.credentials_loaded == 1)
        update_item_list(win);
}

