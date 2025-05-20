#ifndef _NODE_DATA_DEF
#define _NODE_DATA_DEF

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>
#include <lv2/worker/worker.h>
#include <pipewire/filter.h>
#include <pipewire/pipewire.h>
#include <spa/utils/ringbuffer.h>
#include <stdint.h>
#include <suil/suil.h>

#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128

// Enable forward references
struct node_data;
struct port_data;
//

struct pw_data {
   struct pw_thread_loop *loop;
   struct pw_filter *filter;
   int64_t clock_time;
   int connected;
   int samplerate;
   int latency_period;
};

struct lv2_data {
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

   int start_ui;
   SuilInstance *suil_instance;
};

typedef enum {
   CONTROL_INPUT,
   CONTROL_OUTPUT,
   AUDIO_INPUT,
   AUDIO_OUTPUT,
   ATOM_INPUT,
   ATOM_OUTPUT
} port_type;

struct control_input_port {
   float current;
};

struct control_output_port {
   float current;
};

struct audio_input_port {};

struct audio_output_port {};

struct atom_input_port {
   LV2_Atom_Sequence *buffer;
   struct spa_ringbuffer ring;
   uint8_t *ringbuffer;
};

struct atom_output_port {
   LV2_Atom_Sequence *buffer;
   //   struct spa_ringbuffer ring;
   //   uint8_t *ringbuffer;
};

#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
#define MAX_ATOM_MESSAGE_SIZE 256

struct port_data {
   int index;
   port_type type;
   char name[100];
   float dfault;
   float min;
   float max;
   const LilvPort *lilvPort;
   void *pwPort;
   struct pw_buffer *pwbuffer;
   union {
      struct control_input_port control_input;
      struct control_output_port control_output;
      struct audio_input_port audio_input;
      struct audio_output_port audio_output;
      struct atom_input_port atom_input;
      struct atom_output_port atom_output;
   } variant;
   void (*setup)(struct port_data *port, struct node_data *node);
   void (*pre_run)(struct port_data *port, struct node_data *node, uint64_t frame, float denom,
                   uint64_t n_samples);
   void (*post_run)(struct port_data *port, struct node_data *node);
};

struct node_data {
   char nodename[200];
   char plugin_uri[200];
   char preset_uri[200];
   int64_t clock_time;
   struct pw_data pw;
   struct lv2_data host;
   int n_ports;
   struct port_data ports[100];
};

#endif
