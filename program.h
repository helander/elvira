#include <gtk/gtk.h>
//#include <pthread.h>
#include <pipewire/pipewire.h>

//extern pthread_mutex_t program_lock;
extern struct pw_thread_loop *master_loop;

extern void program_on_window_destroy(GtkWidget *widget, gpointer user_data);

extern void program_init();

extern void program_gtk_main();
