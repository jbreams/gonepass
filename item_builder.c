#include "gonepass.h"
#include <string.h>

static void handle_copy_button(GtkButton * button, gpointer data) {
    GtkEntry * value_field = GTK_ENTRY(data);
    GdkAtom atom = gdk_atom_intern_static_string("CLIPBOARD");

    const char * value = gtk_entry_get_text(value_field);
    GtkClipboard * clipboard = gtk_clipboard_get(atom);
    gtk_clipboard_set_text(clipboard, value, -1);
    gtk_clipboard_store(clipboard);
}

static void handle_reveal_button(GtkButton * button, gpointer data) {
    GtkEntry * value_field = GTK_ENTRY(data);
    gboolean visible = gtk_entry_get_visibility(value_field);
    visible = !visible;
    gtk_entry_set_visibility(value_field, visible);
    if(visible)
        gtk_button_set_label(button, "_Hide");
    else
        gtk_button_set_label(button, "_Reveal");
}

static int process_fields_raw(json_t * input, GtkWidget * container, int * index,
    char * value_key, char * type_key, char * title_key) {

    int row_index = *index, tmp_index;
    json_t * row_obj;
    json_array_foreach(input, tmp_index, row_obj) {
        char * item_value, *designation, *item_type;
        GtkTreeIter cur_item;
        if(json_unpack(row_obj, "{s:s s:s s:s}",
            value_key, &item_value,
            title_key, &designation,
            type_key, &item_type
        ) == -1)
            continue;

        GtkWidget * label_widget = gtk_label_new(designation);
        gtk_widget_set_halign(GTK_WIDGET(label_widget), GTK_ALIGN_END);
        gtk_widget_set_margin_end(GTK_WIDGET(label_widget), 5);
        GtkWidget * value_widget = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(value_widget), item_value);
        gtk_widget_set_hexpand(GTK_WIDGET(value_widget), TRUE);

        g_object_set(G_OBJECT(value_widget),
            "editable", FALSE,
            NULL
        );

        GtkWidget * copy_button = NULL, *reveal_button = NULL;

        if(strcmp(item_type, "P") == 0) {
            gtk_entry_set_visibility(GTK_ENTRY(value_widget), FALSE);
            copy_button = gtk_button_new_with_mnemonic("_Copy");
            g_signal_connect(G_OBJECT(copy_button), "clicked",
                G_CALLBACK(handle_copy_button), value_widget);
            reveal_button = gtk_button_new_with_mnemonic("_Reveal");
            g_signal_connect(G_OBJECT(reveal_button), "clicked",
                G_CALLBACK(handle_reveal_button), value_widget);
        }
        else {
        }

        gtk_grid_attach(GTK_GRID(container), label_widget, 0, row_index, 1, 1);
        gtk_grid_attach(GTK_GRID(container), value_widget, 1, row_index, copy_button == NULL ? 3 : 1, 1);
        if(copy_button) {
            gtk_grid_attach(GTK_GRID(container), copy_button, 2, row_index, 1, 1);
            gtk_grid_attach(GTK_GRID(container), reveal_button, 3, row_index, 1, 1);
        }
        row_index++;
    }
    *index = row_index;
    return 0;
}

static int process_section(json_t * input, GtkWidget * container, int * index) {
    char * section_title;
    json_t * section_fields;
    json_unpack(input, "{ s:o s:s }",
        "fields", &section_fields,
        "title", &section_title
    );

    int row_index = *index;
    GtkWidget * title_label = gtk_label_new(strlen(section_title) > 0 ? section_title : "Details");
    gtk_grid_attach(GTK_GRID(container), title_label, 0, row_index++, 4, 1);
    int rc = process_fields_raw(section_fields, container, &row_index, "v", "k", "t");
    if(rc != 0)
        return rc;
    *index = row_index;
    return 0;
}

static int process_fields(json_t * input, GtkWidget * container, int * index) {
    int row_index = *index;
    GtkWidget * title_label = gtk_label_new("Details");
    gtk_grid_attach(GTK_GRID(container), title_label, 0, row_index++, 4, 1);

    int rc = process_fields_raw(input, container, &row_index, "value", "type", "designation");
    *index = row_index;
    return rc;
}

int process_entries(json_t * input, GtkWidget * container) {
    char * notes_plain = NULL;
    json_t * fields = NULL, *sections = NULL;
    json_unpack(input,
        "{ s?:o s?o s?s }",
        "fields", &fields,
        "sections", &sections,
        "notesPlain", &notes_plain
    );
    int rc = 0;

    GtkWidget * details_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(details_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(details_grid), 3);
    gtk_widget_set_margin_end(GTK_WIDGET(details_grid), 5);
    gtk_widget_set_margin_start(GTK_WIDGET(details_grid), 5);
    gtk_box_pack_start(GTK_BOX(container), details_grid, TRUE, TRUE, 2);
    int row_index = 0;

    if(fields)
        rc = process_fields(fields, details_grid, &row_index);
    else if(sections) {
        json_t * section_obj;
        int section_index;

        json_array_foreach(sections, section_index, section_obj) {
            rc = process_section(section_obj, details_grid, &row_index);
            if(rc != 0)
                break;
        }
    }

    if(rc != 0)
        return rc;

    if(notes_plain) {
        GtkWidget * notes_label = gtk_label_new("Notes");
        gtk_grid_attach(GTK_GRID(details_grid), notes_label, 0, row_index++, 4, 1);

        GtkWidget * notes_field = gtk_text_view_new();
        gtk_widget_set_hexpand(GTK_WIDGET(notes_field), TRUE);
        gtk_widget_set_vexpand(GTK_WIDGET(notes_field), TRUE);
        gtk_grid_attach(GTK_GRID(details_grid), notes_field, 0, row_index++, 4, 1);
        GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(notes_field));
        gtk_text_buffer_set_text(buffer, notes_plain, -1);
        g_object_set(G_OBJECT(notes_field),
            "editable", FALSE,
            NULL
        );
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(notes_field), GTK_WRAP_WORD);
    }

    return 0;
}
