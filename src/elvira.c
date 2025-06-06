/*
 * ============================================================================
 *  File:       elvira.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Main program.
 *      
 * ============================================================================
 */

#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "handler.h"
#include "host.h"
#include "node.h"
#include "ports.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
static void startup() {
   node->filter = NULL;
   host->lilv_preset = NULL;
   host->suil_instance = NULL;
   printf("\nStarting engine %s %s (%s)\n\n", config_nodename, config_plugin_uri,
          config_preset_uri);
   fflush(stdout);
   pw_thread_loop_start(runtime_primary_event_loop);
   pw_thread_loop_start(runtime_worker_event_loop);

   host_setup();

   node_setup();

   ports_setup();

   lilv_instance_activate(host->instance);

   if (config_preset_uri) {
      pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_preset, 0,
                     config_preset_uri, strlen(config_preset_uri), false, NULL);
   }

   if (config_show_ui)
      pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_ui_start, 0, NULL, 0,
                     false, NULL);
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int main(int argc, char **argv) {
   char lv2_path[100];
   sprintf(lv2_path, "%s/.lv2:/usr/lib/lv2", getenv("HOME"));
   setenv("LV2_PATH", lv2_path, 0);
   printf("\nlv2_path [%s]", getenv("LV2_PATH"));
   fflush(stdout);

   // Potentially used when creating presets, so create it here in case
   mkdir("/tmp/elvira", 0777);

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
              "\nUsage: $ elvira <engine-nam>e <plugin-uri> [--showui] [--latency period] [--samplerate rate] "
              "[--preset uri]\n");
      exit(-1);
   }

   gtk_init(&argc, &argv);
   pw_init(&argc, &argv);
   runtime_init();
   startup();

   gtk_main();  // Here we spend the time in the main thread

   pw_deinit();
   return 0;
}
