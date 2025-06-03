#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "constants.h"
#include "engine_types.h"
#include "node/node_types.h"
#include "host/host_types.h"
#include "engine.h"

Engine engine = {0};

int main(int argc, char **argv) {
   char lv2_path[100];
   sprintf(lv2_path, "%s/.lv2:/usr/lib/lv2", getenv("HOME"));
   setenv("LV2_PATH", lv2_path, 0);
   printf("\nlv2_path [%s]", getenv("LV2_PATH"));
   fflush(stdout);

   // Potentially used when creating presets, so create it here in case
   mkdir("/tmp/elvira", 0777);

   engine.host = (Host *)calloc(1, sizeof(Host));
   engine.node = (Node *)calloc(1, sizeof(Node));
   engine.started = false;
   engine_defaults(&engine);

   int pos_arg_cnt = 0;
   bool syntax_error = false;
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--showui")) {
         engine.host->start_ui = true;
      } else if (!strcmp(argv[i], "--latency")) {
         if (i < argc - 1)
            engine.node->latency_period = atoi(argv[++i]);
         else
            syntax_error = true;
      } else if (!strcmp(argv[i], "--samplerate")) {
         if (i < argc - 1)
            engine.samplerate = atoi(argv[++i]);
         else
            syntax_error = true;
      } else if (!strcmp(argv[i], "--preset")) {
         if (i < argc - 1)
            strcpy(engine.preset_uri, argv[++i]);
         else
            syntax_error = true;
      } else {
         if (pos_arg_cnt == 0) {
            if (i < argc) strcpy(engine.enginename, argv[i]);
         } else if (pos_arg_cnt == 1) {
            if (i < argc) strcpy(engine.plugin_uri, argv[i]);
         }
         pos_arg_cnt++;
      }
   }
   if (pos_arg_cnt < 2 || syntax_error) {
      fprintf(stderr,
              "\nUsage: engine-name plugin-uri [--showui] [--latency period] [--samplerate rate] "
              "[--preset uri]\n");
      exit(-1);
   }

   gtk_init(&argc, &argv);
   pw_init(&argc, &argv);

   constants_init();

   printf("\nnow entering engine");
   fflush(stdout);
   engine_entry(&engine);

   printf("\nnow entering gtk loop");
   fflush(stdout);

   gtk_main();
   pw_deinit();
   return 0;
}
