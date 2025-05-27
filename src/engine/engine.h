#include <pipewire/filter.h>

#include "engine_data.h"

extern void engine_defaults(Engine *engine);
extern int engine_entry(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                        size_t size, void *user_data);

extern const struct pw_filter_events engine_filter_events;
