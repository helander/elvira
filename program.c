#include "program.h"

#include <gtk/gtk.h>
#include <pipewire/pipewire.h>

#include "constants.h"

pthread_mutex_t program_lock;

void on_window_destroy(GtkWidget* widget, gpointer user_data) { gtk_main_quit(); }

void program_init() {
   int argc = 0;
   char*** argv = NULL;

   gtk_init(&argc, argv);
   pw_init(&argc, argv);
   initConstants();
   pthread_mutex_init(&program_lock, NULL);
}

void program_gtk_main() { gtk_main(); }
