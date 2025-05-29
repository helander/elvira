#include <pipewire/pipewire.h>

#include "types.h"

extern int host_setup(Engine *engine);
extern int host_on_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                          size_t size, void *user_data);
extern int host_on_save(struct spa_loop *loop, bool async, uint32_t seq, const void *data,
                        size_t size, void *user_data);
