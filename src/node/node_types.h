#pragma once

#include "types.h"


//#include <lilv/lilv.h>
//#include <lv2/atom/atom.h>
//#include <lv2/options/options.h>
//#include <lv2/worker/worker.h>
#include <pipewire/filter.h>
#include <pipewire/pipewire.h>
//#include <spa/utils/ringbuffer.h>
#include <stdbool.h>
#include <stdint.h>
//#include <suil/suil.h>

//#define ATOM_BUFFER_SIZE 16 * 1024

//#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
//#define MAX_ATOM_MESSAGE_SIZE 256

//#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
//#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128


struct Node {
   struct pw_thread_loop *engine_loop;
   struct pw_filter *filter;
   int64_t clock_time;
//   int samplerate;
   int latency_period;
   NodePort *ports;
};


typedef enum {
   NODE_NONE,
   NODE_CONTROL_INPUT,
   NODE_CONTROL_OUTPUT,
   NODE_AUDIO_INPUT,
   NODE_AUDIO_OUTPUT
} NodePortType;

struct NodePort {
   int index;
   NodePortType type;
   struct pw_filter_port *pwPort;
   struct pw_buffer *pwbuffer;
};

