#include "pwfilter.h"

#include <spa/param/latency-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "engine.h"
#include "engine_data.h"

int pwfilter_setup(Engine *engine) {
   char latency[50];

   sprintf(latency, "%d/%d", engine->pw.latency_period, engine->pw.samplerate);
   printf("\nquantum  %d/%d", engine->pw.latency_period, engine->pw.samplerate);

   engine->pw.connected = 0;
   // Create pw engine loop resources. Lock the engine loop
   pw_thread_loop_lock(engine->pw.engine_loop);

   engine->pw.filter = pw_filter_new_simple(
       pw_thread_loop_get_loop(engine->pw.engine_loop), engine->enginename,
       pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Filter",
                         PW_KEY_MEDIA_ROLE, "DSP", PW_KEY_MEDIA_NAME, engine->setname,
                         PW_KEY_NODE_LATENCY, latency, PW_KEY_NODE_ALWAYS_PROCESS, "true", NULL),
       &engine_filter_events, engine);

   uint8_t buffer[1024];
   struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(engine->pw.filter, PW_FILTER_FLAG_RT_PROCESS | PW_FILTER_FLAG_DRIVER,
                         params, 1) < 0) {
      fprintf(stderr, "can't connect\n");
      // pthread_mutex_unlock(&program_lock);
      return -1;
   }

   // map lv2ports => pw ports (and create the required pw ports)
   for (int n = 0; n < engine->n_ports; n++) {
      struct port_data *port = &engine->ports[n];
      if (port->setup) port->setup(port, engine);
   }

   // All pw resources created, release the lock
   pw_thread_loop_unlock(engine->pw.engine_loop);

   printf("\nStartup done for pwfilter [%s]", engine->enginename);
   fflush(stdout);
}
