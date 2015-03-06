#include <gtk/gtk.h>
#include <string.h>
#include "gonepass.h"

struct _GonepassUnlockDialog
{
  GtkDialog parent;
};

struct _GonepassUnlockDialogClass
{
  GtkDialogClass parent_class;
};

typedef struct _GonepassUnlockDialogPrivate GonepassUnlockDialogPrivate;

struct _GonepassUnlockDialogPrivate
{
  GSettings *settings;
  GtkButton * unlock_button;
  GtkEntry * password_field;
};

G_DEFINE_TYPE_WITH_PRIVATE(GonepassUnlockDialog, gonepass_unlock_dialog, GTK_TYPE_DIALOG)

static void
gonepass_unlock_dialog_init (GonepassUnlockDialog *prefs)
{
  GonepassUnlockDialogPrivate *priv;
  priv = gonepass_unlock_dialog_get_instance_private (prefs);
  priv->settings = g_settings_new ("org.gtk.gonepass");
  gtk_widget_init_template (GTK_WIDGET (prefs));
}

gchar * gonepass_unlock_dialog_get_pass(GonepassUnlockDialog *dlg) {
    GonepassUnlockDialogPrivate * priv = gonepass_unlock_dialog_get_instance_private(dlg);
    return gtk_entry_get_text(priv->password_field);
}

static void
gonepass_unlock_dialog_dispose (GObject *object)
{
  GonepassUnlockDialogPrivate *priv;

  priv = gonepass_unlock_dialog_get_instance_private (GONEPASS_UNLOCK_DIALOG (object));
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (gonepass_unlock_dialog_parent_class)->dispose (object);
}

static void
gonepass_unlock_dialog_class_init (GonepassUnlockDialogClass *class)
{
  G_OBJECT_CLASS (class)->dispose = gonepass_unlock_dialog_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                              "/org/gtk/gonepass/unlock.ui");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GonepassUnlockDialog, password_field);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GonepassUnlockDialog, unlock_button);

}

GonepassUnlockDialog *
gonepass_unlock_dialog_new (GonepassAppWindow *win)
{
  return g_object_new (GONEPASS_UNLOCK_DIALOG_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
