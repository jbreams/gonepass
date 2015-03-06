#ifndef __GONEPASS_H
#define __GONEPASS_H

#include <gtk/gtk.h>
#include <jansson.h>

#define GONEPASS_APP_TYPE (gonepass_app_get_type())
#define GONEPASS_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GONEPASS_APP_TYPE, GonepassApp))

typedef struct _GonepassApp GonepassApp;
typedef struct _GonepassAppClass GonepassAppClass;

#define GONEPASS_APP_WINDOW_TYPE (gonepass_app_window_get_type())
#define GONEPASS_APP_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GONEPASS_APP_WINDOW_TYPE, GonepassAppWindow))

typedef struct _GonepassAppWindow GonepassAppWindow;
typedef struct _GonepassAppWindowClass GonepassAppWindowClass;

GType gonepass_app_get_type(void);
GonepassApp * gonepass_app_new(void);

GType gonepass_app_window_get_type(void);
GonepassAppWindow * gonepass_app_window_new(GonepassApp * app);
void gonepass_app_window_open(GonepassAppWindow * win, GFile * file);


#define GONEPASS_PREFS_TYPE (gonepass_prefs_get_type ())
#define GONEPASS_PREFS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GONEPASS_PREFS_TYPE, GonepassPrefs))

typedef struct _GonepassPrefs          GonepassPrefs;
typedef struct _GonepassPrefsClass     GonepassPrefsClass;

GType                   gonepass_prefs_get_type     (void);
GonepassPrefs        *gonepass_prefs_new          (GonepassAppWindow *win);

#define GONEPASS_UNLOCK_DIALOG_TYPE (gonepass_unlock_dialog_get_type ())
#define GONEPASS_UNLOCK_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GONEPASS_UNLOCK_DIALOG_TYPE, GonepassUnlockDialog))

typedef struct _GonepassUnlockDialog          GonepassUnlockDialog;
typedef struct _GonepassUnlockDialogClass     GonepassUnlockDialogClass;

GType                   gonepass_unlock_dialog_get_type     (void);
GonepassUnlockDialog        *gonepass_unlock_dialog_new          (GonepassAppWindow *win);
gchar * gonepass_unlock_dialog_get_pass(GonepassUnlockDialog *dlg);

int decrypt_item(json_t * input, char ** output);
int load_credentials(GonepassAppWindow * win);
void clear_credentials();
#endif
