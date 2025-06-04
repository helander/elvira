#pragma once

#include "common/types.h"
#include "node_types.h"
#include "host_types.h"

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>
#include <lv2/worker/worker.h>
#include <pipewire/filter.h>
#include <pipewire/pipewire.h>
#include <spa/utils/ringbuffer.h>
#include <stdbool.h>
#include <stdint.h>
#include <suil/suil.h>

//#define ATOM_BUFFER_SIZE 16 * 1024

//#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
//#define MAX_ATOM_MESSAGE_SIZE 256

//#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
//#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128

//

typedef enum {
   ENGINE_NONE,
   ENGINE_CONTROL_INPUT,
   ENGINE_CONTROL_OUTPUT,
   ENGINE_AUDIO_INPUT,
   ENGINE_AUDIO_OUTPUT
} EnginePortType;

struct EnginePort {
   EnginePortType type;
   HostPort *host_port;
   NodePort *node_port;
   void (*pre_run)(EnginePort *port, uint64_t frame, float denom,
                   uint64_t n_samples);
   void (*post_run)(EnginePort *port);
   struct spa_ringbuffer ring;  //  not used by all
   uint8_t *ringbuffer;         //  not used by all
};


