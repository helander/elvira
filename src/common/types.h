#pragma once


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


#define ATOM_BUFFER_SIZE 16 * 1024

#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
#define MAX_ATOM_MESSAGE_SIZE 256

#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128


// Enable forward references
typedef struct Engine Engine;
typedef struct Node   Node;
typedef struct Host   Host;
typedef struct NodePort NodePort;
typedef struct HostPort HostPort;
typedef struct EnginePort EnginePort;

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
   void (*pre_run)(EnginePort *port, Engine *engine, uint64_t frame, float denom,
                   uint64_t n_samples);
   void (*post_run)(EnginePort *port, Engine *engine);
   struct spa_ringbuffer ring; //  not used by all
   uint8_t *ringbuffer;        //  not used by all
};


struct Node {
   struct pw_thread_loop *engine_loop;
   struct pw_filter *filter;
   int64_t clock_time;
   int samplerate;
   int latency_period;
   NodePort  *ports;
};

struct Host {
   const LilvPlugin *lilvPlugin;  // byt namn till lilv_plugin
   LilvNode *lilv_preset;
   LilvInstance *instance;  // byt namn till lilv_instance
   LV2_Handle handle;       // byt namn till lv2_handle
   const LV2_Worker_Interface *iface;
   void *worker_data;
   LV2_Worker_Schedule work_schedule;
   LV2_Feature work_schedule_feature;
   LV2_Options_Option options[6];
   LV2_Feature options_feature;
   const LV2_Feature *features[7];
   struct spa_ringbuffer work_response_ring;
   uint8_t work_response_buffer[WORK_RESPONSE_RINGBUFFER_SIZE];
   int32_t block_length;
   bool start_ui;
   SuilInstance *suil_instance;
   HostPort *ports;
};

typedef enum {
   HOST_NONE,
   HOST_CONTROL_INPUT,
   HOST_CONTROL_OUTPUT,
   HOST_AUDIO_INPUT,
   HOST_AUDIO_OUTPUT,
   HOST_ATOM_INPUT,
   HOST_ATOM_OUTPUT
} HostPortType;


struct HostPort {
   int index;
   HostPortType type;
   char name[100];
   const LilvPort *lilvPort;
   float dfault;              // not used by all
   float min;                 // not used by all
   float max;                 // not used by all
   LV2_Atom_Sequence *buffer; // not used by all
   float current;             // not used by all
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


struct Engine {
   bool started;
   char setname[50];
   char enginename[50];
   char plugin_uri[200];
   char preset_uri[200];
   Node node;
   Host host;
   EnginePort *ports;
};

