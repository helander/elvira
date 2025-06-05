#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "node.h"
#include "runtime.h"
#include "host.h"




#include "handler.h"
//#include "engine_types.h"
#include "ports.h"
#include "host.h"
#include "node.h"

#include "runtime.h"
#include "ui.h"

#include <lilv/lilv.h>
#include <set.h>


static
void startup() {
   node->filter = NULL;
   host->lilv_preset = NULL;
   host->suil_instance = NULL;
   printf("\nStarting engine %s %s (%s)\n\n", config_nodename, config_plugin_uri,config_preset_uri);fflush(stdout);
   pw_thread_loop_start(runtime_primary_event_loop);
   pw_thread_loop_start(runtime_worker_event_loop);

   printf("\nhost_setup()");fflush(stdout);

   host_setup();

   printf("\nnode_setup()");fflush(stdout);
   node_setup();

   printf("\nports_setup()");fflush(stdout);
   ports_setup();

   lilv_instance_activate(host->instance);  // create host_activate() and call it?

   // embed this in a function host_apply_preset (can we make host indep of engine and only
   // host, we could then pass loop with the call)
   if (config_preset_uri) {
      pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_preset, 0,
                     config_preset_uri, strlen(config_preset_uri), false, NULL);
   }

   if (config_show_ui)
      pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), pluginui_on_start, 0, NULL,
                     0, false, NULL);
}







int main(int argc, char **argv) {
   char lv2_path[100];
   sprintf(lv2_path, "%s/.lv2:/usr/lib/lv2", getenv("HOME"));
   setenv("LV2_PATH", lv2_path, 0);
   printf("\nlv2_path [%s]", getenv("LV2_PATH"));
   fflush(stdout);

   // Potentially used when creating presets, so create it here in case
   mkdir("/tmp/elvira", 0777);

   //engine_defaults();


   int pos_arg_cnt = 0;
   bool syntax_error = false;
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--showui")) {
         config_show_ui = true;
      } else if (!strcmp(argv[i], "--latency")) {
         if (i < argc - 1)
            config_latency_period = atoi(argv[++i]);
         else
            syntax_error = true;
      } else if (!strcmp(argv[i], "--samplerate")) {
         if (i < argc - 1)
            config_samplerate = atoi(argv[++i]);
         else
            syntax_error = true;
      } else if (!strcmp(argv[i], "--preset")) {
         if (i < argc - 1)
            config_preset_uri = strdup(argv[++i]);
         else
            syntax_error = true;
      } else {
         if (pos_arg_cnt == 0) {
            if (i < argc) 
               config_nodename = strdup(argv[i]);
            else
               syntax_error = true;
         } else if (pos_arg_cnt == 1) {
             config_plugin_uri = strdup(argv[i]);
         }
         pos_arg_cnt++;
      }
   }
   if (config_nodename == NULL || config_plugin_uri == NULL) syntax_error = true;
   if (pos_arg_cnt < 2 || syntax_error) {
      fprintf(stderr,
              "\nUsage: engine-name plugin-uri [--showui] [--latency period] [--samplerate rate] "
              "[--preset uri]\n");
      exit(-1);
   }

   printf("\ngtk_init()");fflush(stdout);
   gtk_init(&argc, &argv);
   printf("\npw_init()");fflush(stdout);
   pw_init(&argc, &argv);
   printf("\nruntime_init()");fflush(stdout);
   runtime_init();
   printf("\nstartup()");fflush(stdout);
   startup();
   printf("\ngtk_main()");fflush(stdout);

   gtk_main();
   printf("\npw_deinit()");fflush(stdout);
   pw_deinit();
   return 0;
}
