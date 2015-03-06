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
    GtkTreeView * item_entries_list;
    GtkListStore * item_list_store;
    GtkTreeModel * item_list_sorter;
    GtkTreeModel * item_list_filterer;
    GtkCellRenderer * item_list_renderer;
    GtkLabel * item_name;
    GtkTextBuffer * notes_buffer;
    GtkTextView * notes_view;
    GtkExpander * notes_expander;

    struct credentials_bag bag;
};

G_DEFINE_TYPE_WITH_PRIVATE(GonepassAppWindow, gonepass_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void update_item_list(GonepassAppWindow * win) {
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    char vault_path[PATH_MAX];
    snprintf(vault_path, PATH_MAX, "%s/data/default/contents.js", priv->bag.vault_path);
    json_error_t json_err;
    json_t * contents_js = json_load_file(vault_path, 0, &json_err);
    if(contents_js == NULL) {
        fprintf(stderr, "Error loading contents.js: %s", json_err.text);
        return;
    }

    size_t contents_index;
    json_t * cur_item;
    GtkListStore * model = priv->item_list_store;

    json_array_foreach(contents_js, contents_index, cur_item) {
        char * name, *uuid, *type;
        GtkTreeIter iter;
        json_unpack(cur_item, "[s s s]", &name, &type, &uuid);
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

    json_t * item_json = json_load_file(item_file_path, 0, &json_err);
    if(item_json == NULL) {
        printf("Cannot load item file! %s\n", json_err.text);
        return;
    }
    g_free(uuid);

    char * decrypted_payload;
    int payload_size = decrypt_item(item_json, &priv->bag, &decrypted_payload);
    json_decref(item_json);

    json_t * decrypted_item = json_loadb(decrypted_payload, payload_size, 0, &json_err);
    if(decrypted_item == NULL) {
        printf("Error loading decrypted JSON: %s\n", json_err.text);
        return;
    }

    const char *item_name;
    char * notes_plain = NULL;
    json_t * fields = NULL, *sections = NULL;
    json_unpack(decrypted_item,
        "{ s?:o s?o s?s }",
        "fields", &fields,
        "sections", &sections,
        "notesPlain", &notes_plain
    );

    GtkTreeStore * treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    if(fields) {
        json_t *value;
        int index;
        json_array_foreach(fields, index, value) {
            char * item_value, *designation, *item_type;
            GtkTreeIter cur_item;
            if(json_unpack(value, "{s:s s:s s:s}",
                "value", &item_value,
                "designation", &designation,
                "type", &item_type
            ) == -1)
                continue;

            gtk_tree_store_append(treestore, &cur_item, NULL);
            gtk_tree_store_set(treestore, &cur_item,
                0, designation,
                1, item_value,
                -1
            );
        }
    }
    else if(sections) {
        json_t *section_obj;
        int index;
        json_array_foreach(sections, index, section_obj) {
            char * section_title;
            json_t * section_fields, *cur_item;
            GtkTreeIter section_iter;
            json_unpack(section_obj, "{ s:o s:s }",
                "fields", &section_fields,
                "title", &section_title
            );
            if(strlen(section_title) > 0) {
                gtk_tree_store_append(treestore, &section_iter, NULL);
                gtk_tree_store_set(treestore, &section_iter, 0, section_title, -1);
            } else
                section_title = NULL;

            int sub_index;
            json_array_foreach(section_fields, sub_index, cur_item) {
                GtkTreeIter child_iter;
                char * item_title, *item_value, *item_type;
                if(json_unpack(cur_item, "{s:s s:s s:s}",
                    "t", &item_title,
                    "v", &item_value,
                    "k", &item_type) != 0)
                    continue;
                gtk_tree_store_append(treestore, &child_iter,
                    section_title != NULL ? &section_iter : NULL);
                gtk_tree_store_set(treestore, &child_iter,
                    0, item_title,
                    1, item_value,
                    -1);
            }
        }
    }

    if(notes_plain) {
        gtk_text_buffer_set_text(priv->notes_buffer, notes_plain, -1);
        gtk_expander_set_expanded(priv->notes_expander, TRUE);
    } else {
        gtk_text_buffer_set_text(priv->notes_buffer, "", -1);
        gtk_expander_set_expanded(priv->notes_expander, FALSE);
    }

    gtk_tree_view_set_model(priv->item_entries_list, GTK_TREE_MODEL(treestore));
    gtk_tree_view_expand_all(priv->item_entries_list);
    json_decref(decrypted_item);
}

static gboolean right_click_event_cb(GtkWidget * widget, GdkEvent * event, gpointer data) {
    GonepassAppWindow * win = GONEPASS_APP_WINDOW(data);
    GonepassAppWindowPrivate * priv = gonepass_app_window_get_instance_private(win);
    guint button;
    gdk_event_get_button(event, &button);
    gdouble x = 0, y = 0;

    if(button != GDK_BUTTON_SECONDARY || gdk_event_get_coords(event, &x, &y) == FALSE)
        return TRUE;

    GtkTreePath * path;
    GtkTreeViewColumn * column;
    gint cellx, celly;

    if(!gtk_tree_view_get_path_at_pos(priv->item_entries_list,
        x,
        y,
        &path,
        &column,
        &cellx,
        &celly
    ))
        return TRUE;

    GtkTreeIter iter;
    GtkTreeModel * model = gtk_tree_view_get_model(priv->item_entries_list);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    const char * value;
    gtk_tree_model_get(model, &iter, 1, &value, -1);

    GdkAtom atom = gdk_atom_intern_static_string("CLIPBOARD");

    GtkClipboard * clipboard = gtk_clipboard_get(atom);
    gtk_clipboard_set_text(clipboard, value, -1);
    gtk_clipboard_store(clipboard);

    return TRUE;
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

    load_credentials(win, &priv->bag);
    update_item_list(win);

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

    column = gtk_tree_view_column_new_with_attributes(
        "Name",
        renderer,
        "text", 0,
        NULL);
    gtk_tree_view_append_column(priv->item_entries_list, column);

    column = gtk_tree_view_column_new_with_attributes(
        "Value",
        renderer,
        "text", 1,
        NULL);
    gtk_tree_view_append_column(priv->item_entries_list, column);

    g_signal_connect(G_OBJECT(priv->item_search), "search-changed", G_CALLBACK(item_search_changed), win);
    g_signal_connect(G_OBJECT(priv->item_entries_list), "button-press-event", G_CALLBACK(right_click_event_cb), win);
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
        GonepassAppWindow, item_entries_list);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, item_name);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, notes_view);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, notes_buffer);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
        GonepassAppWindow, notes_expander);

}

GonepassAppWindow * gonepass_app_window_new(GonepassApp * app) {
    return g_object_new(GONEPASS_APP_WINDOW_TYPE, "application", app, NULL);
}

void gonepass_app_window_open(GonepassAppWindow * win, GFile * file) {
    // don't do anything yet!
}
