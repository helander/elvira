#include "runtime.h"

#include <pipewire/pipewire.h>
#include <stdbool.h>

struct pw_thread_loop *runtime_primary_event_loop;
struct pw_thread_loop *runtime_worker_event_loop;

int config_samplerate = 48000;
int config_latency_period = 512;
bool config_show_ui = false;
char *config_plugin_uri = NULL;
char *config_preset_uri = NULL;
char *config_nodename = NULL;

void runtime_init() {
   runtime_primary_event_loop = pw_thread_loop_new("primary", NULL);
   runtime_worker_event_loop = pw_thread_loop_new("worker", NULL);
}
