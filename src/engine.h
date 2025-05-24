#include "engine_data.h"
#include <pipewire/filter.h>

extern void engine_defaults(Engine *engine);
extern int  engine_entry(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size, void *user_data);

extern const struct pw_filter_events engine_filter_events;
/*
extern void node_create(struct node_data *node);
extern void node_destroy(struct node_data *node);
extern void node_set_plugin(struct node_data *node, const char *plugin_uri);
extern void node_set_preset(struct node_data *node, const char *preset_uri);
extern void node_set_name(struct node_data *node, const char *node_name);
extern void node_set_samplerate(struct node_data *node, int samplerate);
extern void node_set_latency(struct node_data *node, int period);
extern void node_set_ui_show(struct node_data *node, int show_ui);
//extern void node_start(struct node_data *node);
*/
