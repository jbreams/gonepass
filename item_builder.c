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

static int process_fields_raw(json_t * input, GtkWidget * container,
    char * value_key, char * type_key, char * title_key) {
    GtkWidget * details_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(container), details_grid);

    gtk_grid_set_row_spacing(GTK_GRID(details_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(details_grid), 2);

    int row_index;
    json_t * row_obj;
    json_array_foreach(input, row_index, row_obj) {
        char * item_value, *designation, *item_type;
        GtkTreeIter cur_item;
        if(json_unpack(row_obj, "{s:s s:s s:s}",
            value_key, &item_value,
            title_key, &designation,
            type_key, &item_type
        ) == -1)
            continue;

        GtkWidget * label_widget = gtk_label_new(designation);
        gtk_label_set_width_chars(GTK_LABEL(label_widget), 20);
        gtk_widget_set_halign(label_widget, GTK_ALIGN_START);
        GtkWidget * value_widget = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(value_widget), item_value);

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
            gtk_entry_set_width_chars(GTK_ENTRY(value_widget), 40);
        }
        else {
            gtk_entry_set_width_chars(GTK_ENTRY(value_widget), 65);
        }

        gtk_grid_attach(GTK_GRID(details_grid), label_widget, 0, row_index, 1, 1);
        gtk_grid_attach(GTK_GRID(details_grid), value_widget, 1, row_index, copy_button == NULL ? 3 : 1, 1);
        if(copy_button) {
            gtk_grid_attach(GTK_GRID(details_grid), copy_button, 2, row_index, 1, 1);
            gtk_grid_attach(GTK_GRID(details_grid), reveal_button, 3, row_index, 1, 1);
        }
    }
    return 0;
}

static int process_section(json_t * input, GtkWidget * container) {
    char * section_title;
    json_t * section_fields;
    json_unpack(input, "{ s:o s:s }",
        "fields", &section_fields,
        "title", &section_title
    );

    char * section_title_mnemonic;
    size_t title_len = strlen(section_title);
    if(title_len == 0)
        section_title_mnemonic = strdup("Details");
    else {
        section_title_mnemonic = calloc(1, strlen(section_title) + 2);
        sprintf(section_title_mnemonic, "_%s", section_title);
    }
    GtkWidget * expander = gtk_expander_new_with_mnemonic(section_title_mnemonic);
    free(section_title_mnemonic);

    int rc = process_fields_raw(section_fields, expander, "v", "k", "t");
    if(rc != 0)
        return rc;

    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
    //gtk_container_add(GTK_CONTAINER(container), expander);
    gtk_box_pack_start(GTK_BOX(container), expander, FALSE, FALSE, 2);
    return 0;
}

static int process_fields(json_t * input, GtkWidget * container) {
    GtkWidget * expander = gtk_expander_new_with_mnemonic("_Details");
    int rc = process_fields_raw(input, expander, "value", "type", "designation");
    if(rc != 0)
        return rc;

    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
    //gtk_container_add(GTK_CONTAINER(container), expander);
    gtk_box_pack_start(GTK_BOX(container), expander, FALSE, FALSE, 2);
    return 0;
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
    int rc;

    if(fields)
        rc = process_fields(fields, container);
    else if(sections) {
        json_t * section_obj;
        int section_index;

        json_array_foreach(sections, section_index, section_obj) {
            rc = process_section(section_obj, container);
            if(rc != 0)
                break;
        }
    }

    if(rc != 0)
        return rc;

    if(notes_plain) {
        GtkWidget * expander = gtk_expander_new_with_mnemonic("_Notes");
        GtkWidget * notes_field = gtk_text_view_new();
        gtk_container_add(GTK_CONTAINER(expander), notes_field);
        GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(notes_field));
        gtk_text_buffer_set_text(buffer, notes_plain, -1);
        g_object_set(G_OBJECT(notes_field),
            "editable", FALSE,
            NULL
        );
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(notes_field), GTK_WRAP_WORD);
        gtk_widget_set_valign(notes_field, GTK_ALIGN_FILL);
        gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
        gtk_box_pack_start(GTK_BOX(container), expander, TRUE, TRUE, 2);
    }

    return 0;
}
