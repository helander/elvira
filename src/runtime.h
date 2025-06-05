#include <stdbool.h>
#include <pipewire/pipewire.h>

extern struct pw_thread_loop *runtime_primary_event_loop;
extern struct pw_thread_loop *runtime_worker_event_loop;

extern int config_samplerate;
extern int config_latency_period;
extern bool config_show_ui;
extern char *config_plugin_uri;
extern char *config_preset_uri;
extern char *config_nodename;


extern void runtime_init();
