
#include "engine.h"

#include <spa/debug/types.h>
#include <spa/pod/iter.h>
#include "engine_data.h"
#include "ui.h"
#include "host.h"
#include "pwfilter.h"



static void on_process(void *userdata, struct spa_io_position *position) {
   Engine *engine = userdata;

   if (!engine->pw.connected) {
      // Defer start of plugin UI until any engine port is connected
      engine->pw.connected = 1;
      if (engine->host.start_ui)
         pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.engine_loop), pluginui_on_start, 0, NULL, 0,
                        false, engine);
   }

   uint32_t n_samples = position->clock.duration;
   uint64_t frame = engine->clock_time;
   float denom = (float)position->clock.rate.denom;
   engine->clock_time += position->clock.duration;

   for (int n = 0; n < engine->n_ports; n++) {
      struct port_data *port = &engine->ports[n];
      if (port->pre_run) port->pre_run(port, engine, frame, denom, (uint64_t)n_samples);
   }

   lilv_instance_run(engine->host.instance, n_samples);
   /////
   {
      struct spa_ringbuffer *ring = &engine->host.work_response_ring;
      uint8_t *buffer = engine->host.work_response_buffer;
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
         engine->host.iface->work_response(engine->host.handle, msg_len, payload);

         read_index += sizeof(uint16_t) + msg_len;
         spa_ringbuffer_read_update(ring, read_index);
      }
   }
   /////

   for (int n = 0; n < engine->n_ports; n++) {
      struct port_data *port = &engine->ports[n];
      if (port->post_run) port->post_run(port, engine);
   }
}


static void on_command(void *data, const struct spa_command *command) {
   Engine *engine = (Engine *)data;
   if (SPA_NODE_COMMAND_ID(command) == SPA_NODE_COMMAND_User) {
      if (SPA_POD_TYPE(&command->pod) == SPA_TYPE_Object) {
         const struct spa_pod_object *obj = (const struct spa_pod_object *) &command->pod;
         struct spa_pod_prop *prop;
         SPA_POD_OBJECT_FOREACH(obj, prop) {
             if (prop->key == SPA_COMMAND_NODE_extra) {
                const struct spa_pod *value = &prop->value;
                if (SPA_POD_TYPE(value) == SPA_TYPE_String) {
                   const char *command_string = SPA_POD_BODY(value);

      printf("\nCommand---[%s]", command_string);

      char uri[100];
      if (sscanf(command_string, "preset %s", uri) == 1) {
         pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.master_loop), host_on_preset, 0, uri, strlen(uri) + 1,
                        false, engine);
      } if (sscanf(command_string, "save %s", uri) == 1) {
         pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.master_loop), host_on_save, 0, uri, strlen(uri) + 1,
                        false, engine);
      } else {
         printf("\nUnknown command [%s]", command_string);
      }

                }
             }
         }
      }
   }
} 



static void on_param_changed(void *data, void *port_data, uint32_t id, const struct spa_pod *param) {
   printf("\nParam changed type %d  size %d", param->type, param->size);
   if (param->type == SPA_TYPE_Object) printf("\nobject");
   fflush(stdout);
}

static void on_filter_destroy(void *data) {
   Engine *engine = (Engine *)data;
   if (engine->pw.filter) pw_filter_destroy(engine->pw.filter);
}

const struct pw_filter_events engine_filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
    .command = on_command,
    .destroy = on_filter_destroy,
    .param_changed = on_param_changed,
};


void engine_defaults(Engine *engine) {
   engine->groupname[0] = 0;
   engine->enginename[0] = 0;
   engine->plugin_uri[0] = 0;
   engine->preset_uri[0] = 0;
   engine->host.start_ui = true;
   engine->pw.samplerate = 48000;
   engine->pw.latency_period = 512;
}

#if 0
void node_destroy(struct node_data *node) {
   printf("\nTermination of node [%s]", node->nodename);
   fflush(stdout);
   pw_thread_loop_destroy(node->pw.node_loop);

   node->pw.node_loop = NULL;
   node->pw.filter = NULL;
   node->nodename[0] = 0;
   node->plugin_uri[0] = 0;
   node->preset_uri[0] = 0;
   node->host.lilv_preset = NULL;
   node->host.suil_instance = NULL;
}


#endif

int engine_entry(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size, void *user_data) {
   Engine *engine = (Engine *)user_data;

   engine->pw.filter = NULL;
   engine->host.lilv_preset = NULL;
   engine->host.suil_instance = NULL;
   engine->pw.engine_loop = pw_thread_loop_new("engine", NULL);
//   engine->pw.engine_loop = engine->pw.master_loop;
   pw_thread_loop_start(engine->pw.engine_loop);

   printf("\nStarting engine %s in group %s",engine->enginename, engine->groupname);fflush(stdout);

   host_setup(engine);
   pwfilter_setup(engine);

   lilv_instance_activate(engine->host.instance); //create host_activate() and call it?

   //embed this in a function host_apply_preset (can we make host indep of engine and only engine->host, we could then pass loop with the call)
   pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.master_loop), host_on_preset, 0, engine->preset_uri,
                  sizeof(engine->preset_uri), false, engine);

   printf("\nStartup done for engine [%s]", engine->enginename);
   fflush(stdout);
}

