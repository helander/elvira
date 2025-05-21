#include "program.h"

#include <gtk/gtk.h>
#include <pipewire/pipewire.h>

#include "constants.h"

#include "node_data.h"
#include "node.h"

extern struct node_data n1, n2;

//pthread_mutex_t program_lock;

struct pw_thread_loop *master_loop = NULL;

void on_window_destroy(GtkWidget* widget, gpointer user_data) { gtk_main_quit(); }




static void do_quit(void *userdata, int signal_number) {
   // Destroy all node thread loops and then run pw_deinit

   printf("\nTerminate elvira\n");fflush(stdout);
   pw_thread_loop_stop(master_loop);                                                                                                                                                        
   pw_thread_loop_destroy(master_loop);
   node_destroy(&n1);
   node_destroy(&n2);
   pw_deinit();
   printf("\n elvira terminated\n");fflush(stdout);
   exit(0);
}

void program_init() {
   int argc = 0;
   char*** argv = NULL;

   gtk_init(&argc, argv);
   pw_init(&argc, argv);
   initConstants();
//   pthread_mutex_init(&program_lock, NULL);
   master_loop = pw_thread_loop_new("master", NULL);
   pw_loop_add_signal(pw_thread_loop_get_loop(master_loop), SIGINT, do_quit, "master int");
   pw_loop_add_signal(pw_thread_loop_get_loop(master_loop), SIGTERM, do_quit, "master term");
}

void program_gtk_main() { 
   gtk_main();
   pw_deinit();
 }
