#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <stdio.h>

#include "utils/constants.h"
#include "enginegroup.h"
#include "controller/controller.h"


static char *empty_group = "{\"group\": \"\", \"engines\": []}";

char *read_stream_to_string(FILE *stream) {
   if (!stream) return NULL;
   size_t size = 4096, len = 0;
   char *data = malloc(size);
   if (!data) return NULL;
   int c;
   while ((c = fgetc(stream)) != EOF) {
      if (len + 1 >= size) {
         size *= 2;
         char *new_data = realloc(data, size);
         if (!new_data) {
            free(data);
            return NULL;
         }
         data = new_data;
      }
      data[len++] = (char)c;
   }
   data[len] = '\0';
   return data;
}


//struct pw_thread_loop *master_loop;

/*
static void do_quit(void *userdata, int signal_number) {
   printf("\nTerminate elvira\n");
   fflush(stdout);
   pw_thread_loop_stop(master_loop);
   pw_thread_loop_destroy(master_loop);
   controller_destroy();
   pw_deinit();
   printf("\n elvira terminated\n");
   fflush(stdout);
   exit(0);
}


   //   master_loop = pw_thread_loop_new("master", NULL);
   //   pw_loop_add_signal(pw_thread_loop_get_loop(master_loop), SIGINT, do_quit, "master int");
   //   pw_loop_add_signal(pw_thread_loop_get_loop(master_loop), SIGTERM, do_quit, "master term");
   //   pw_thread_loop_start(master_loop);



*/

int main(int argc, char **argv) {
   gtk_init(&argc, &argv);
   pw_init(&argc, &argv);
   constants_init();
   controller_init();

   FILE *jsonfile = NULL;
   if (argc > 1) {
      fprintf(stderr, "\nOpen file %s", argv[1]);
      jsonfile = fopen(argv[1], "r");
   } else {
      fprintf(stderr, "\nOpen default file (elvira.json)");
      jsonfile = fopen("elvira.json", "r");
   }

   char *json_text = NULL;

   if (jsonfile == NULL) {
      perror("\nNo file available:");
   } else {
     json_text = read_stream_to_string(jsonfile);
   }

   if (json_text == NULL) {
     fprintf(stderr, "\nNo file content available, use empty group.");
     json_text = empty_group;
   }

   EngineGroup *engines = enginegroup_parse(json_text);
   if (json_text != empty_group) free(json_text);

   if (engines) {
     controller_add(engines);
     enginegroup_free(engines);
     controller_start();
     gtk_main();
     pw_deinit();
     return 0;
   }
   fprintf(stderr,"Failed to parse json input.\n");
}
