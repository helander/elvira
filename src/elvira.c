#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <pipewire/pipewire.h>

#include "controller/controller.h"
#include "set.h"
#include "utils/constants.h"





static char *empty_set = "{\"set\": \"\", \"engines\": []}";

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
      fprintf(stderr, "\nNo file content available, use empty set.");
      json_text = empty_set;
   }

   // Potentially used when creating presets, so create it here in case
   mkdir("/tmp/elvira", 0777);

   EngineSet *engines = engineset_parse(json_text);
   if (json_text != empty_set) free(json_text);

   if (engines) {
      controller_add(engines);
      engineset_free(engines);
      controller_start();
      gtk_main();
      pw_deinit();
      return 0;
   }
   fprintf(stderr, "Failed to parse json input.\n");
}
