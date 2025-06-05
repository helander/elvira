#pragma once

#include <stdint.h>

#include "host.h"
#include "node.h"
#include "set.h"

#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
#define MAX_ATOM_MESSAGE_SIZE 256

typedef struct Port Port;

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
   void (*pre_run)(Port *port, uint64_t frame, float denom, uint64_t n_samples);
   void (*post_run)(Port *port);
   struct spa_ringbuffer ring;  //  not used by all
   uint8_t *ringbuffer;         //  not used by all
};

extern void ports_setup();

extern void pre_run_audio_input(Port *port, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_audio_input(Port *port);
extern void pre_run_audio_output(Port *port, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_audio_output(Port *port);
extern void pre_run_control_input(Port *port, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_control_input(Port *port);
extern void pre_run_control_output(Port *port, uint64_t frame, float denom, uint64_t n_samples);
extern void post_run_control_output(Port *port);

void ports_write(void *const controller, const uint32_t port_index, const uint32_t buffer_size,
                 const uint32_t protocol, const void *const buffer);

extern Set ports;
