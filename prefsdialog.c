#include <gtk/gtk.h>
#include <string.h>
#include "gonepass.h"

extern char vault_path[PATH_MAX];

struct _GonepassPrefs
{
  GtkDialog parent;
};

struct _GonepassPrefsClass
{
  GtkDialogClass parent_class;
};

typedef struct _GonepassPrefsPrivate GonepassPrefsPrivate;

struct _GonepassPrefsPrivate
{
  GSettings *settings;
  GtkFileChooserButton *vault_chooser;
};

G_DEFINE_TYPE_WITH_PRIVATE(GonepassPrefs, gonepass_prefs, GTK_TYPE_DIALOG)

static void
gonepass_prefs_choose_vault(GtkFileChooserButton * widget, gpointer data) {
    GonepassPrefs * win = GONEPASS_PREFS(data);
    GonepassPrefsPrivate * priv = gonepass_prefs_get_instance_private(win);
    gchar * cur_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    g_settings_set_string(priv->settings, "vault-path", cur_dir);
}

static void
gonepass_prefs_init (GonepassPrefs *prefs)
{
  GonepassPrefsPrivate *priv;
  priv = gonepass_prefs_get_instance_private (prefs);
  priv->settings = g_settings_new ("org.gtk.gonepass");
  gtk_widget_init_template (GTK_WIDGET (prefs));
  g_signal_connect(G_OBJECT(priv->vault_chooser), "file-set",
      G_CALLBACK(gonepass_prefs_choose_vault), prefs);
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->vault_chooser),
      g_settings_get_string(priv->settings, "vault-path"));
}

static void
gonepass_prefs_dispose (GObject *object)
{
  GonepassPrefsPrivate *priv;

  priv = gonepass_prefs_get_instance_private (GONEPASS_PREFS (object));
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (gonepass_prefs_parent_class)->dispose (object);
}

static void
gonepass_prefs_class_init (GonepassPrefsClass *class)
{
  G_OBJECT_CLASS (class)->dispose = gonepass_prefs_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/gonepass/prefs.ui");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GonepassPrefs, vault_chooser);

}

GonepassPrefs *
gonepass_prefs_new (GonepassAppWindow *win)
{
  return g_object_new (GONEPASS_PREFS_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
