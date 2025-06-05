#include "handler.h"
#include "host.h"
#include "node.h"
#include "ports.h"

static void process_work_responses() {
   struct spa_ringbuffer *ring = &host->work_response_ring;
   uint8_t *buffer = host->work_response_buffer;
   uint32_t read_index;
   uint32_t write_index;
   spa_ringbuffer_get_read_index(ring, &read_index);
   spa_ringbuffer_get_write_index(ring, &write_index);
   while ((write_index - read_index) >= sizeof(uint16_t)) {
      uint16_t msg_len;
      uint32_t offset = read_index & (WORK_RESPONSE_RINGBUFFER_SIZE - 1);
      uint32_t space = WORK_RESPONSE_RINGBUFFER_SIZE - offset;

      if (space >= sizeof(uint16_t)) {
         memcpy(&msg_len, buffer + offset, sizeof(uint16_t));
      } else {
         uint8_t tmp[2];
         memcpy(tmp, buffer + offset, space);
         memcpy(tmp + space, buffer, sizeof(uint16_t) - space);
         memcpy(&msg_len, tmp, sizeof(uint16_t));
      }

      if ((write_index - read_index) < sizeof(uint16_t) + msg_len) {
         break;  // Incomplete message
      }

      uint8_t payload[MAX_WORK_RESPONSE_MESSAGE_SIZE];
      offset = (read_index + sizeof(uint16_t)) & (WORK_RESPONSE_RINGBUFFER_SIZE - 1);
      space = WORK_RESPONSE_RINGBUFFER_SIZE - offset;

      if (space >= msg_len) {
         memcpy(payload, buffer + offset, msg_len);
      } else {
         memcpy(payload, buffer + offset, space);
         memcpy(payload + space, buffer, msg_len - space);
      }
      // printf("\nCall work_response from on_process");fflush(stdout);
      if (host->iface && host->iface->work_response)
         host->iface->work_response(host->handle, msg_len, payload);

      read_index += sizeof(uint16_t) + msg_len;
      spa_ringbuffer_read_update(ring, read_index);
   }
}

void on_process(void *userdata, struct spa_io_position *position) {
   uint32_t n_samples = position->clock.duration;
   uint64_t frame = node->clock_time;
   float denom = (float)position->clock.rate.denom;
   node->clock_time += position->clock.duration;

   Port *port;

   SET_FOR_EACH(Port *, port, &ports) {
      if (port->pre_run) {
         port->pre_run(port, frame, denom, (uint64_t)n_samples);
      }
   }

   lilv_instance_run(host->instance, n_samples);

   process_work_responses();
   if (host->iface && host->iface->end_run) host->iface->end_run(host->handle);

   SET_FOR_EACH(Port *, port, &ports) {
      if (port->post_run) port->post_run(port);
   }
}
