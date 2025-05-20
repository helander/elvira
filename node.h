#include "node_data.h"

// extern void *node_plugin(const char *pluginUri);
// extern void *node_preset(const void *plugin, const char *presetUri);
// extern void node_instance(void *plugin, void *preset, char *instanceName, int sampleRate, int
// period); extern void node_thread_loop(void *plugin, void *preset, char *instanceName, int
// sampleRate, int period);

extern void node_init(struct node_data *node);
extern void node_set_plugin(struct node_data *node, const char *plugin_uri);
extern void node_set_preset(struct node_data *node, const char *preset_uri);
extern void node_set_name(struct node_data *node, const char *node_name);
extern void node_set_samplerate(struct node_data *node, int samplerate);
extern void node_set_latency(struct node_data *node, int period);
extern void node_set_ui_show(struct node_data *node, int show_ui);
extern void node_start(struct node_data *node);
