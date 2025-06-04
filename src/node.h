#include <pipewire/pipewire.h>

#include "common/types.h"

extern Node *node;

extern struct pw_filter_events *node_get_engine_filter_events();
extern int node_setup();
