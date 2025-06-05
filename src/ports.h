#pragma once

#include <stdint.h>
//#include "types.h"
#include "set.h"
#include "host.h"
#include "node.h"

typedef struct Port Port;

extern Set ports;

extern void ports_setup();

extern void pre_run_audio_input(Port *port,  uint64_t frame, float denom,
                                uint64_t n_samples);
extern void post_run_audio_input(Port *port);
extern void pre_run_audio_output(Port *port,  uint64_t frame, float denom,
                                 uint64_t n_samples);
extern void post_run_audio_output(Port *port);
extern void pre_run_control_input(Port *port, uint64_t frame, float denom,
                                  uint64_t n_samples);
extern void post_run_control_input(Port *port);
extern void pre_run_control_output(Port *port, uint64_t frame, float denom,
                                   uint64_t n_samples);
extern void post_run_control_output(Port *port);


/*
//#include "types.h"
#include "host.h"
#include "node.h"

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
*/

#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
#define MAX_ATOM_MESSAGE_SIZE 256


typedef enum {
   PORT_NONE,
   PORT_CONTROL_INPUT,
   PORT_CONTROL_OUTPUT,
   PORT_AUDIO_INPUT,
   PORT_AUDIO_OUTPUT
} PortType;

struct Port {
   PortType type;
   HostPort *host_port;
   NodePort *node_port;
   void (*pre_run)(Port *port, uint64_t frame, float denom,
                   uint64_t n_samples);
   void (*post_run)(Port *port);
   struct spa_ringbuffer ring;  //  not used by all
   uint8_t *ringbuffer;         //  not used by all
};


