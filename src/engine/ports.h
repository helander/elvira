#include "engine_data.h"

extern void ports_init(Engine* engine);
extern void ports_write_port(void* const controller, const uint32_t port_index,
                             const uint32_t buffer_size, const uint32_t protocol,
                             const void* const buffer);
