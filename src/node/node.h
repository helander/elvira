#include <pipewire/pipewire.h>

#include "common/types.h"

extern struct pw_filter_events *node_get_engine_filter_events();
extern int node_setup(Engine *engine);
