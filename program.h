#include <gtk/gtk.h>
#include <pthread.h>

extern pthread_mutex_t program_lock;

extern void program_on_window_destroy(GtkWidget *widget, gpointer user_data);

extern void program_init();

extern void program_gtk_main();
