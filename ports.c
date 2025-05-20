#include "ports.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include <spa/control/control.h>
#include <spa/control/ump-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "constants.h"
#include "node_data.h"
#include "util.h"


static float dummyAudioInput[20000];
static float dummyAudioOutput[20000];

#define ATOM_BUFFER_SIZE 16 * 1024

// seq is used to pass the port index and data passes the atom
static int on_port_event_aseq(struct spa_loop *loop, bool async, uint32_t port_index, const void *data, size_t size,
                     void *user_data) {
  //printf("\nPort %d send aseq",port_index);
  //util_print_atom_sequence(aseq);
  struct node_data *node = (struct node_data *) user_data;
  LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *) data;
  if (node->host.suil_instance) {



   LV2_Atom_Event *aev = (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
   if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
      long payloadSize = aseq->atom.size;
      while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
         suil_instance_port_event(node->host.suil_instance, port_index, aev->body.size, constants.atom_eventTransfer, &aev->body);
         int eventSize =
             lv2_atom_pad_size(sizeof(LV2_Atom_Event)) + lv2_atom_pad_size(aev->body.size);
         char *next = ((char *)aev) + eventSize;
         payloadSize = payloadSize - eventSize;
         aev = (LV2_Atom_Event *)next;
      }
   }

  }
}

static void send_atom_sequence(int port_index, LV2_Atom_Sequence *aseq, struct node_data *node) {
   pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_port_event_aseq, port_index, aseq, aseq->atom.size + sizeof(LV2_Atom), false, node);
}

// seq is used to pass the port index and data passes the atom
static int on_port_event_atom(struct spa_loop *loop, bool async, uint32_t port_index, const void *atom, size_t size,
                     void *user_data) {
  //printf("\nPort %d send atom",port_index);
  //util_print_atom(atom);
  struct node_data *node = (struct node_data *) user_data;
  if (node->host.suil_instance)
     suil_instance_port_event(node->host.suil_instance, port_index, size, constants.atom_eventTransfer, atom);
}

static void send_atom(int port_index, LV2_Atom *atom, struct node_data *node) {
  //char prefix[30];
   pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_port_event_atom, port_index, atom, atom->size + sizeof(LV2_Atom), false, node);
}

//============================= control input
//==========================================================

static void setup_control_input(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   this->variant.control_input.current = this->dfault;
   //printf("\nControl input port %d (%s) start value: %f", this->index, this->name, this->variant.control_input.current);
   //fflush(stdout);
   lilv_instance_connect_port(instance, this->index, &this->variant.control_input.current);
   this->pwPort = pw_filter_add_port(
       filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
       pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32", PW_KEY_PORT_NAME, this->name, "port.hidden", "true", NULL),
       NULL, 0);
}


static void post_run_control_input(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   // printf("\nControl input port %d
   // %f",this->index,this->variant.control_input.current);fflush(stdout);
}

static void init_control_input(struct port_data *this) {
   this->type = CONTROL_INPUT;
   this->setup = setup_control_input;
   this->post_run = post_run_control_input;
}

//============================= control output
//==========================================================

static void setup_control_output(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   lilv_instance_connect_port(instance, this->index, &this->variant.control_output.current);
   this->pwPort = pw_filter_add_port(
       filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
       pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32", PW_KEY_PORT_NAME, this->name, NULL),
       NULL, 0);
}

static void init_control_output(struct port_data *this) {
   this->type = CONTROL_OUTPUT;
   this->setup = setup_control_output;
}

//============================ audio input
//===========================================================

static void setup_audio_input(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   this->pwPort = pw_filter_add_port(filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                                     pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                                       PW_KEY_PORT_NAME, this->name, NULL),
                                     NULL, 0);
}

static void pre_run_audio_input(struct port_data *this, struct node_data *node, uint64_t frame,
                                float denom, uint64_t n_samples) {
   LilvInstance *instance = node->host.instance;
   float *inp = pw_filter_get_dsp_buffer(this->pwPort, n_samples);
   if (inp == NULL) {
      lilv_instance_connect_port(instance, this->index, dummyAudioOutput);
   } else {
      lilv_instance_connect_port(instance, this->index, inp);
   }
}

static void init_audio_input(struct port_data *this) {
   this->type = AUDIO_INPUT;
   this->setup = setup_audio_input;
   this->pre_run = pre_run_audio_input;
}

//============================= audio output
//==========================================================

static void setup_audio_output(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   this->pwPort =
       pw_filter_add_port(filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                          pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                            PW_KEY_PORT_NAME, this->name, NULL),
                          NULL, 0);
}

static void pre_run_audio_output(struct port_data *this, struct node_data *node, uint64_t frame,
                                 float denom, uint64_t n_samples) {
   LilvInstance *instance = node->host.instance;
   float *outp = pw_filter_get_dsp_buffer(this->pwPort, n_samples);
   if (outp == NULL) {
      lilv_instance_connect_port(instance, this->index, dummyAudioOutput);
   } else {
      lilv_instance_connect_port(instance, this->index, outp);
   }
}

static void init_audio_output(struct port_data *this) {
   this->type = AUDIO_OUTPUT;
   this->setup = setup_audio_output;
   this->pre_run = pre_run_audio_output;
}

//====================== atom input
//=================================================================

static void setup_atom_input(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   this->variant.atom_input.buffer = calloc(1, ATOM_BUFFER_SIZE);
   this->variant.atom_input.ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
   spa_ringbuffer_init(&this->variant.atom_input.ring);
   lilv_instance_connect_port(instance, this->index, this->variant.atom_input.buffer);

   this->pwPort = pw_filter_add_port(
       filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
       pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME, this->name, NULL),
       NULL, 0);
}

static void pre_run_atom_input(struct port_data *this, struct node_data *node, uint64_t frame,
                               float denom, uint64_t n_samples) {
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)this->variant.atom_input.buffer;
   aseq->atom.size = ATOM_BUFFER_SIZE - sizeof(LV2_Atom);
   lv2_atom_sequence_clear(aseq);
   aseq->atom.type = constants.atom_Sequence;

   {
      uint32_t read_index;
      uint32_t write_index;
      spa_ringbuffer_get_read_index(&this->variant.atom_input.ring, &read_index);
      spa_ringbuffer_get_write_index(&this->variant.atom_input.ring, &write_index);
      while  ((write_index - read_index) >= sizeof(uint16_t)) {
         // printf("\ndiff write index %x   read_index
         // %x",write_index,read_index);fflush(stdout);
         uint16_t msg_len;
         uint32_t offset = read_index & (ATOM_RINGBUFFER_SIZE - 1);
         uint32_t space = ATOM_RINGBUFFER_SIZE - offset;

         if (space >= sizeof(uint16_t)) {
            memcpy(&msg_len, this->variant.atom_input.ringbuffer + offset, sizeof(uint16_t));
         } else {
            uint8_t tmp[2];
            memcpy(tmp, this->variant.atom_input.ringbuffer + offset, space);
            memcpy(tmp + space, this->variant.atom_input.ringbuffer, sizeof(uint16_t) - space);
            memcpy(&msg_len, tmp, sizeof(uint16_t));
         }

         if ((write_index - read_index) < sizeof(uint16_t) + msg_len) {
            break;  // Incomplete message
         }

         uint8_t payload[MAX_ATOM_MESSAGE_SIZE];
         offset = (read_index + sizeof(uint16_t)) & (ATOM_RINGBUFFER_SIZE - 1);
         space = ATOM_RINGBUFFER_SIZE - offset;

         if (space >= msg_len) {
            memcpy(payload, this->variant.atom_input.ringbuffer + offset, msg_len);
         } else {
            memcpy(payload, this->variant.atom_input.ringbuffer + offset, space);
            memcpy(payload + space, this->variant.atom_input.ringbuffer, msg_len - space);
         }

         // printf("RT received message (%u bytes): ", msg_len);fflush(stdout);
         // for (int i = 0; i < msg_len; i++) {
         //     printf("%02X ", payload[i]);
         // }
         // printf("\n");

         read_index += sizeof(uint16_t) + msg_len;
         spa_ringbuffer_read_update(&this->variant.atom_input.ring, read_index);
         LV2_Atom *atom = (LV2_Atom *)payload;
         if (ATOM_BUFFER_SIZE - sizeof(LV2_Atom) - aseq->atom.size >=
             sizeof(LV2_Atom_Event) + atom->size + sizeof(LV2_Atom)) {
            LV2_Atom_Event *aev =
                (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
            aev->time.frames = 0;
            aev->body.type = atom->type;
            aev->body.size = atom->size;
            memcpy(LV2_ATOM_BODY(&aev->body), &atom[1], atom->size);

            int size = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + atom->size);
            aseq->atom.size += size;
         }
         break; //limit to one message per run 
         //        spa_ringbuffer_get_read_index(&this->variant.atom_input.ring, &read_index);
         //        spa_ringbuffer_get_write_index(&this->variant.atom_input.ring, &write_index);
         // printf("\nafter read update write index %x   read_index
         // %x",write_index,read_index);fflush(stdout);
      }
   }

   util_print_atom_sequence(node->nodename, "input", this->index, aseq);

   this->pwbuffer = pw_filter_dequeue_buffer(this->pwPort);

   if (!this->pwbuffer) return;

   struct spa_buffer *buf;
   struct spa_data *d;
   struct spa_pod *pod;
   struct spa_pod_control *c;
   buf = this->pwbuffer->buffer;
   d = &buf->datas[0];
   if ((pod = spa_pod_from_data(d->data, d->maxsize, d->chunk->offset, d->chunk->size)) != NULL) {
      if (spa_pod_is_sequence(pod)) {
         int buf_offset = 0;
         SPA_POD_SEQUENCE_FOREACH((struct spa_pod_sequence *)pod, c) {
            if (c->type != SPA_CONTROL_UMP) {
               pw_log_error("Unhandled type %d [ump = %d]", c->type, SPA_CONTROL_UMP);
               continue;
            }
            uint8_t *ump_data = SPA_POD_BODY(&c->value);
            size_t ump_size = SPA_POD_BODY_SIZE(&c->value);
            uint8_t midi_data[8];
            int midi_size =
                spa_ump_to_midi((uint32_t *)ump_data, ump_size, midi_data, sizeof(midi_data));
            if (midi_data[0] == 0xf8) continue;
            if (ATOM_BUFFER_SIZE - sizeof(LV2_Atom) - aseq->atom.size >=
                sizeof(LV2_Atom_Event) + midi_size) {
               LV2_Atom_Event *aev =
                   (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq) +
                                      buf_offset);
               aev->time.frames = (frame + c->offset) / denom;
               aev->body.type = constants.midi_MidiEvent;
               aev->body.size = midi_size;
               memcpy(LV2_ATOM_BODY(&aev->body), midi_data, midi_size);

               int size = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + midi_size);
               aseq->atom.size += size;
               buf_offset += size;
            }
         }
      } else {
         pw_log_error("Pod is not sequence port %d", this->index);
      }
   } else {
      pw_log_error("No pod for port %d", this->index);
      printf("No pod for port %d", this->index);
      fflush(stdout);
   }
}

static void post_run_atom_input(struct port_data *this, struct node_data *node) {
   //LilvInstance *instance = node->host.instance;
   if (this->pwbuffer) pw_filter_queue_buffer(this->pwPort, this->pwbuffer);
}

static void init_atom_input(struct port_data *this) {
   this->type = ATOM_INPUT;
   this->setup = setup_atom_input;
   this->pre_run = pre_run_atom_input;
   this->post_run = post_run_atom_input;
}

//============================ atom output
//===========================================================

static void setup_atom_output(struct port_data *this, struct node_data *node) {
   LilvInstance *instance = node->host.instance;
   struct pw_filter *filter = node->pw.filter;
   this->variant.atom_output.buffer = calloc(1, ATOM_BUFFER_SIZE);
   //this->variant.atom_output.ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
   //spa_ringbuffer_init(&this->variant.atom_output.ring);
   lilv_instance_connect_port(instance, this->index, this->variant.atom_output.buffer);
   this->pwPort = pw_filter_add_port(
       filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
       pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME, this->name, NULL),
       NULL, 0);
}

static void pre_run_atom_output(struct port_data *this, struct node_data *node, uint64_t frame,
                                float denom, uint64_t n_samples) {
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)this->variant.atom_output.buffer;
   aseq->atom.size = ATOM_BUFFER_SIZE - sizeof(LV2_Atom);
   aseq->atom.type = constants.atom_Chunk;

   this->pwbuffer = pw_filter_dequeue_buffer(this->pwPort);
}

static void post_run_atom_output(struct port_data *this, struct node_data *node) {
   //LilvInstance *instance = node->host.instance;
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)this->variant.atom_output.buffer;
   util_print_atom_sequence(node->nodename, "output", this->index, aseq);
   send_atom_sequence(this->index, aseq, node);
   //if (!this->pwbuffer) return;
   LV2_Atom_Event *aev = (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
   if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
      struct spa_data *d;
      struct spa_pod_builder builder;
      struct spa_pod_frame frame;
      if (this->pwbuffer) {
         spa_assert(this->pwbuffer->buffer->n_datas == 1);
         d = &this->pwbuffer->buffer->datas[0];
         d->chunk->offset = 0;
         d->chunk->size = 0;
         d->chunk->stride = 1;
         d->chunk->flags = 0;
         spa_pod_builder_init(&builder, d->data, d->maxsize);
         spa_pod_builder_push_sequence(&builder, &frame, 0);
      }
      long payloadSize = aseq->atom.size;
      while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
         if (this->pwbuffer && aev->body.type == constants.midi_MidiEvent) {
            uint8_t *mididata = (uint8_t *)aev + sizeof(LV2_Atom_Event);
            spa_pod_builder_control(&builder, 0, SPA_CONTROL_Midi);
            spa_pod_builder_bytes(&builder, mididata, aev->body.size);
         }
         //send_atom(this->index,&aev->body, node);
         int eventSize =
             lv2_atom_pad_size(sizeof(LV2_Atom_Event)) + lv2_atom_pad_size(aev->body.size);
         char *next = ((char *)aev) + eventSize;
         payloadSize = payloadSize - eventSize;
         aev = (LV2_Atom_Event *)next;
      }
      if (this->pwbuffer) {
        spa_pod_builder_pop(&builder, &frame);
        d->chunk->size = builder.state.offset;
      }
   }
   if (this->pwbuffer) pw_filter_queue_buffer(this->pwPort, this->pwbuffer);
}

static void init_atom_output(struct port_data *this) {
   this->type = ATOM_OUTPUT;
   this->setup = setup_atom_output;
   this->pre_run = pre_run_atom_output;
   this->post_run = post_run_atom_output;
}

void ports_init(struct node_data *node) {
   const LilvPlugin *plugin = node->host.lilvPlugin;
   char *nodename = node->nodename;
   //printf("\nports_init");
   fflush(stdout);
   memset(dummyAudioInput, 0, sizeof(dummyAudioInput));

   int n_ports = lilv_plugin_get_num_ports(plugin);
   node->n_ports = n_ports;
   for (int n = 0; n < n_ports; n++) {
      struct port_data *port = &node->ports[n];
      port->index = n;
      //printf("\ninit_ports port %d", port->index);
      fflush(stdout);

      port->lilvPort = lilv_plugin_get_port_by_index(plugin, n);
      strcpy(port->name, lilv_node_as_string(lilv_port_get_symbol(plugin, port->lilvPort)));
      port->pwPort = NULL;
      port->setup = NULL;
      port->pre_run = NULL;
      port->post_run = NULL;
      if (lilv_port_is_a(plugin, port->lilvPort, constants.atom_AtomPort) &&
          lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         init_atom_input(port);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.atom_AtomPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         init_atom_output(port);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         LilvNode *lv2_default = lilv_new_uri(constants.world, LILV_NS_LV2 "default");
         LilvNode *default_val = lilv_port_get(plugin, port->lilvPort, lv2_default);
         if (default_val) {
            port->dfault = lilv_node_as_float(default_val);
         }
         init_control_input(port);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         init_control_output(port);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_AudioPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         init_audio_input(port);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_AudioPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         init_audio_output(port);
      } else {
         printf("\nUnsupported port type: port #%d (%s)", port->index, port->name);
      }
   }
}

void ports_write_port(void *const controller, const uint32_t port_index, const uint32_t buffer_size,
                    const uint32_t protocol, const void *const buffer) {
   struct node_data *node = (struct node_data *)controller;
   if (protocol == 0U) {
      const float value = *(const float *)buffer;
      printf("\nWrite to control port %d value %f", port_index, value);
      fflush(stdout);
      //  do something here ...
   } else if (protocol == constants.atom_eventTransfer) {
      const LV2_Atom *const atom = (const LV2_Atom *)buffer;
      if (buffer_size < sizeof(LV2_Atom) || (sizeof(LV2_Atom) + atom->size != buffer_size)) {
         printf("\nWrite to atom port %d canceled - wrong buffer size %d", port_index, buffer_size);
         fflush(stdout);
      } else {
         //printf("\n[%s]  Write to atom port %d - buffer size %d atom size %d  type %d %s",
         //       node->nodename, port_index, buffer_size, atom->size, atom->type,
         //       constants_unmap(constants, atom->type));
         //fflush(stdout);
         struct port_data *port = &node->ports[port_index];

         uint16_t len = buffer_size;
         if (buffer_size > MAX_ATOM_MESSAGE_SIZE) {
            fprintf(stderr, "Payload too large\n");
         } else {
            uint8_t temp[MAX_ATOM_MESSAGE_SIZE + sizeof(uint16_t)];
            memcpy(temp, &len, sizeof(uint16_t));
            memcpy(temp + sizeof(uint16_t), buffer, len);
            uint32_t total_len = len + sizeof(uint16_t);

            uint32_t write_index;
            spa_ringbuffer_get_write_index(&port->variant.atom_input.ring, &write_index);

            uint32_t ring_offset = write_index & (ATOM_RINGBUFFER_SIZE - 1);
            uint32_t space = ATOM_RINGBUFFER_SIZE - ring_offset;

            if (space >= total_len) {
               memcpy(port->variant.atom_input.ringbuffer + ring_offset, temp, total_len);
            } else {
               // Wrap around
               memcpy(port->variant.atom_input.ringbuffer + ring_offset, temp, space);
               memcpy(port->variant.atom_input.ringbuffer, temp + space, total_len - space);
            }

            spa_ringbuffer_write_update(&port->variant.atom_input.ring, write_index + total_len);
         }
      }
   }
}


