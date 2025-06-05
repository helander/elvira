#include "ports.h"

#include <spa/control/control.h>
#include <spa/control/ump-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "constants.h"
#include "handler.h"
#include "host.h"
#include "node.h"
#include "runtime.h"
#include "set.h"
#include "types.h"

Set ports;

static float dummyAudioInput[20000];
static float dummyAudioOutput[20000];

static void send_atom_sequence(int port_index, LV2_Atom_Sequence *aseq) {
   pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_port_event_aseq,
                  port_index, aseq, aseq->atom.size + sizeof(LV2_Atom), false, NULL);
}

static void send_atom(int port_index, LV2_Atom *atom) {
   pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_port_event_atom,
                  port_index, atom, atom->size + sizeof(LV2_Atom), false, NULL);
}

void pre_run_audio_input(Port *port, uint64_t frame, float denom, uint64_t n_samples) {
   float *inp = pw_filter_get_dsp_buffer(port->node_port->pwPort, n_samples);
   if (inp == NULL) {
      lilv_instance_connect_port(host->instance, port->host_port->index, dummyAudioInput);
   } else {
      lilv_instance_connect_port(host->instance, port->host_port->index, inp);
   }
}

void post_run_audio_input(Port *port) {}

void pre_run_audio_output(Port *port, uint64_t frame, float denom, uint64_t n_samples) {
   float *outp = pw_filter_get_dsp_buffer(port->node_port->pwPort, n_samples);
   if (outp == NULL) {
      lilv_instance_connect_port(host->instance, port->host_port->index, dummyAudioOutput);
   } else {
      lilv_instance_connect_port(host->instance, port->host_port->index, outp);
   }
}

void post_run_audio_output(Port *port) {}

void pre_run_control_input(Port *port, uint64_t frame, float denom, uint64_t n_samples) {
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)port->host_port->buffer;
   aseq->atom.size = ATOM_BUFFER_SIZE - sizeof(LV2_Atom);
   lv2_atom_sequence_clear(aseq);
   aseq->atom.type = constants.atom_Sequence;

   {
      uint32_t read_index;
      uint32_t write_index;
      spa_ringbuffer_get_read_index(&port->ring, &read_index);
      spa_ringbuffer_get_write_index(&port->ring, &write_index);
      while ((write_index - read_index) >= sizeof(uint16_t)) {
         // printf("\ndiff write index %x   read_index
         // %x",write_index,read_index);fflush(stdout);
         uint16_t msg_len;
         uint32_t offset = read_index & (ATOM_RINGBUFFER_SIZE - 1);
         uint32_t space = ATOM_RINGBUFFER_SIZE - offset;

         if (space >= sizeof(uint16_t)) {
            memcpy(&msg_len, port->ringbuffer + offset, sizeof(uint16_t));
         } else {
            uint8_t tmp[2];
            memcpy(tmp, port->ringbuffer + offset, space);
            memcpy(tmp + space, port->ringbuffer, sizeof(uint16_t) - space);
            memcpy(&msg_len, tmp, sizeof(uint16_t));
         }

         if ((write_index - read_index) < sizeof(uint16_t) + msg_len) {
            break;  // Incomplete message
         }

         uint8_t payload[MAX_ATOM_MESSAGE_SIZE];
         offset = (read_index + sizeof(uint16_t)) & (ATOM_RINGBUFFER_SIZE - 1);
         space = ATOM_RINGBUFFER_SIZE - offset;

         if (space >= msg_len) {
            memcpy(payload, port->ringbuffer + offset, msg_len);
         } else {
            memcpy(payload, port->ringbuffer + offset, space);
            memcpy(payload + space, port->ringbuffer, msg_len - space);
         }

         read_index += sizeof(uint16_t) + msg_len;
         spa_ringbuffer_read_update(&port->ring, read_index);
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
         break;  // limit to one message per run
         //        spa_ringbuffer_get_read_index(&port->ring, &read_index);
         //        spa_ringbuffer_get_write_index(&port->ring, &write_index);
      }
   }

   port->node_port->pwbuffer = pw_filter_dequeue_buffer(port->node_port->pwPort);

   if (!port->node_port->pwbuffer) return;

   struct spa_buffer *buf;
   struct spa_data *d;
   struct spa_pod *pod;
   struct spa_pod_control *c;
   buf = port->node_port->pwbuffer->buffer;
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
         pw_log_error("Pod is not sequence port %d", port->host_port->index);
      }
   } else {
      pw_log_error("No pod for port %d", port->host_port->index);
   }
}

void post_run_control_input(Port *port) {
   if (port->node_port->pwbuffer)
      pw_filter_queue_buffer(port->node_port->pwPort, port->node_port->pwbuffer);
}

void pre_run_control_output(Port *port, uint64_t frame, float denom, uint64_t n_samples) {
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)port->host_port->buffer;
   aseq->atom.size = ATOM_BUFFER_SIZE - sizeof(LV2_Atom);
   aseq->atom.type = constants.atom_Chunk;
   port->node_port->pwbuffer = pw_filter_dequeue_buffer(port->node_port->pwPort);
}

void post_run_control_output(Port *port) {
   LV2_Atom_Sequence *aseq = (LV2_Atom_Sequence *)port->host_port->buffer;
   send_atom_sequence(port->host_port->index, aseq);
   LV2_Atom_Event *aev = (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
   if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
      struct spa_data *d;
      struct spa_pod_builder builder;
      struct spa_pod_frame frame;
      if (port->node_port->pwbuffer) {
         spa_assert(this->pwbuffer->buffer->n_datas == 1);
         d = &port->node_port->pwbuffer->buffer->datas[0];
         d->chunk->offset = 0;
         d->chunk->size = 0;
         d->chunk->stride = 1;
         d->chunk->flags = 0;
         spa_pod_builder_init(&builder, d->data, d->maxsize);
         spa_pod_builder_push_sequence(&builder, &frame, 0);
      }
      long payloadSize = aseq->atom.size;
      while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
         if (port->node_port->pwbuffer && aev->body.type == constants.midi_MidiEvent) {
            uint8_t *mididata = (uint8_t *)aev + sizeof(LV2_Atom_Event);
            spa_pod_builder_control(&builder, 0, SPA_CONTROL_Midi);
            spa_pod_builder_bytes(&builder, mididata, aev->body.size);
         }
         int eventSize =
             lv2_atom_pad_size(sizeof(LV2_Atom_Event)) + lv2_atom_pad_size(aev->body.size);
         char *next = ((char *)aev) + eventSize;
         payloadSize = payloadSize - eventSize;
         aev = (LV2_Atom_Event *)next;
      }
      if (port->node_port->pwbuffer) {
         spa_pod_builder_pop(&builder, &frame);
         d->chunk->size = builder.state.offset;
      }
   }
   if (port->node_port->pwbuffer)
      pw_filter_queue_buffer(port->node_port->pwPort, port->node_port->pwbuffer);
}

void ports_setup() {
   memset(dummyAudioInput, 0, sizeof(dummyAudioInput));
   set_init(&ports);
   NodePort *node_port;
   SET_FOR_EACH(NodePort *, node_port, &node->ports) {
      HostPort *host_port = NULL;
      HostPort *hport;
      SET_FOR_EACH(HostPort *, hport, &host->ports) {
         if (hport->index == node_port->index) {
            host_port = hport;
            break;
         }
      }
      Port *port = (Port *)calloc(1, sizeof(Port));
      port->host_port = host_port;
      port->node_port = node_port;
      switch (node_port->type) {
         case NODE_CONTROL_INPUT:
            port->type = PORT_CONTROL_INPUT;
            port->ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
            spa_ringbuffer_init(&port->ring);
            port->pre_run = pre_run_control_input;
            port->post_run = post_run_control_input;
            break;
         case NODE_CONTROL_OUTPUT:
            port->type = PORT_CONTROL_OUTPUT;
            port->ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
            spa_ringbuffer_init(&port->ring);
            port->pre_run = pre_run_control_output;
            port->post_run = post_run_control_output;
            break;
         case NODE_AUDIO_INPUT:
            port->type = PORT_AUDIO_INPUT;
            port->pre_run = pre_run_audio_input;
            port->post_run = post_run_audio_input;
            break;
         case NODE_AUDIO_OUTPUT:
            port->type = PORT_AUDIO_OUTPUT;
            port->pre_run = pre_run_audio_output;
            port->post_run = post_run_audio_output;
            break;
         default:
            free(port);
            port = NULL;
      }
      if (port) set_add(&ports, port);
   }
}

static Port *find_port(uint32_t port_index) {
   Port *port;
   SET_FOR_EACH(Port *, port, &ports) {
      if (!port->host_port) continue;
      if (port->host_port->index == port_index) {
         return port;
      }
   }
   return NULL;
}

void ports_write(void *const controller, const uint32_t port_index, const uint32_t buffer_size,
                 const uint32_t protocol, const void *const buffer) {
   Port *port = find_port(port_index);
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
         // printf("\n[%s]  Write to atom port %d - buffer size %d atom size %d  type %d %s",
         //        engine->enginename, port_index, buffer_size, atom->size, atom->type,
         //        constants_unmap(constants, atom->type));
         // fflush(stdout);

         uint16_t len = buffer_size;
         if (buffer_size > MAX_ATOM_MESSAGE_SIZE) {
            fprintf(stderr, "Payload too large\n");
         } else {
            uint8_t temp[MAX_ATOM_MESSAGE_SIZE + sizeof(uint16_t)];
            memcpy(temp, &len, sizeof(uint16_t));
            memcpy(temp + sizeof(uint16_t), buffer, len);
            uint32_t total_len = len + sizeof(uint16_t);

            uint32_t write_index;
            spa_ringbuffer_get_write_index(&port->ring, &write_index);

            uint32_t ring_offset = write_index & (ATOM_RINGBUFFER_SIZE - 1);
            uint32_t space = ATOM_RINGBUFFER_SIZE - ring_offset;

            if (space >= total_len) {
               memcpy(port->ringbuffer + ring_offset, temp, total_len);
            } else {
               // Wrap around
               memcpy(port->ringbuffer + ring_offset, temp, space);
               memcpy(port->ringbuffer, temp + space, total_len - space);
            }

            spa_ringbuffer_write_update(&port->ring, write_index + total_len);
         }
      }
   }
}
