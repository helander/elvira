
#include "engine.h"

#include <spa/debug/types.h>
#include <spa/pod/iter.h>

#include "common/types.h"
#include "engine_types.h"
#include "host_types.h"
#include "node_types.h"

#include "engine_ports.h"
#include "host.h"
#include "ui.h"
#include "node.h"



static void process_work_responses()

{
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

static void on_process(void *userdata, struct spa_io_position *position) {
   //   printf("   ONP   ");fflush(stdout);
   //Engine *engine = userdata;

   uint32_t n_samples = position->clock.duration;
   uint64_t frame = node->clock_time;
   float denom = (float)position->clock.rate.denom;
   node->clock_time += position->clock.duration;

   EnginePort *port;

   SET_FOR_EACH(EnginePort*, port, &engine_ports) {
      if (port->pre_run) {
         port->pre_run(port, frame, denom, (uint64_t)n_samples);
      }
   }

   lilv_instance_run(host->instance, n_samples);

   process_work_responses();
   if (host->iface && host->iface->end_run)
      host->iface->end_run(host->handle);

   SET_FOR_EACH(EnginePort*, port, &engine_ports) {
      if (port->post_run) port->post_run(port);
   }

}

static void on_command(void *data, const struct spa_command *command) {
   //Engine *engine = (Engine *)data;
   if (SPA_NODE_COMMAND_ID(command) == SPA_NODE_COMMAND_User) {
      if (SPA_POD_TYPE(&command->pod) == SPA_TYPE_Object) {
         const struct spa_pod_object *obj = (const struct spa_pod_object *)&command->pod;
         struct spa_pod_prop *prop;
         SPA_POD_OBJECT_FOREACH(obj, prop) {
            if (prop->key == SPA_COMMAND_NODE_extra) {
               const struct spa_pod *value = &prop->value;
               if (SPA_POD_TYPE(value) == SPA_TYPE_String) {
                  const char *command_string = SPA_POD_BODY(value);
                  char args[100];
                  if (sscanf(command_string, "preset %s", args) == 1) {
                     pw_loop_invoke(pw_thread_loop_get_loop(node->engine_loop),
                                    host_on_preset, 0, args, strlen(args) + 1, false, NULL);
                  } else if (sscanf(command_string, "save %s", args) == 1) {
                     pw_loop_invoke(pw_thread_loop_get_loop(node->engine_loop), host_on_save,
                                    0, args, strlen(args) + 1, false, NULL);
                  } else {
                     printf("\nUnknown command [%s]", command_string);
                  }
               }
            }
         }
      }
   }
}

static void on_param_changed(void *data, void *port_data, uint32_t id,
                             const struct spa_pod *param) {
   printf("\nParam changed type %d  size %d", param->type, param->size);
   if (param->type == SPA_TYPE_Object) printf("\nobject");
   fflush(stdout);
}

static void on_filter_destroy(void *data) {
   if (node->filter) pw_filter_destroy(node->filter);
}

/*
const struct pw_filter_events engine_filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
    .command = on_command,
    .destroy = on_filter_destroy,
    .param_changed = on_param_changed,
};
*/

void engine_defaults() {
   node->nodename[0] = 0;
   host->plugin_uri[0] = 0;
   host->preset_uri[0] = 0;
   host->start_ui = false;
   node->samplerate = 48000;
   node->latency_period = 512;
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
   node->host->lilv_preset = NULL;
   node->host->suil_instance = NULL;
}

#endif

static void engine_ports_setup() {
   //engine_ports = NULL;
   set_init(&engine_ports);
   NodePort *node_port;
   SET_FOR_EACH(NodePort*, node_port, &node->ports) {
   //for (int n = 0; n < arrlen(node->ports); n++) {
      //NodePort *node_port = &node->ports[n];
      HostPort *host_port = NULL;
      HostPort *port;
      SET_FOR_EACH(HostPort*, port, &host->ports) {
      //for (int n = 0; n < arrlen(host->ports); n++) {
         //HostPort *port = &host->ports[n];
         if (port->index == node_port->index) {
            host_port = port;
            break;
         }
      }
      printf("\nEP %s %d", host_port->name, node_port->type);
      fflush(stdout);
      EnginePort *engine_port = (EnginePort *)calloc(1, sizeof(EnginePort));
      engine_port->host_port = host_port;
      engine_port->node_port = node_port;
      switch (node_port->type) {
         case NODE_CONTROL_INPUT:
            engine_port->type = ENGINE_CONTROL_INPUT;
            engine_port->ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
            spa_ringbuffer_init(&engine_port->ring);
            engine_port->pre_run = pre_run_control_input;
            engine_port->post_run = post_run_control_input;
            break;
         case NODE_CONTROL_OUTPUT:
            engine_port->type = ENGINE_CONTROL_OUTPUT;
            engine_port->ringbuffer = calloc(1, ATOM_RINGBUFFER_SIZE);
            spa_ringbuffer_init(&engine_port->ring);
            engine_port->pre_run = pre_run_control_output;
            engine_port->post_run = post_run_control_output;
            break;
         case NODE_AUDIO_INPUT:
            engine_port->type = ENGINE_AUDIO_INPUT;
            engine_port->pre_run = pre_run_audio_input;
            engine_port->post_run = post_run_audio_input;
            break;
         case NODE_AUDIO_OUTPUT:
            engine_port->type = ENGINE_AUDIO_OUTPUT;
            engine_port->pre_run = pre_run_audio_output;
            engine_port->post_run = post_run_audio_output;
            break;
         default:
            free(engine_port);
            engine_port = NULL;
      }
      //if (engine_port) arrput(engine_ports, *engine_port);
      if (engine_port) set_add(&engine_ports, engine_port);
   }
}

void engine_entry() {
   node->filter = NULL;
   host->lilv_preset = NULL;
   host->suil_instance = NULL;
   node->engine_loop = pw_thread_loop_new("engine", NULL);
   pw_thread_loop_start(node->engine_loop);

   printf("\nStarting engine %s %s (%s)\n\n", node->nodename, host->plugin_uri,
          host->preset_uri);
   fflush(stdout);

   host_setup();

   node_get_engine_filter_events()->process = on_process;
   node_get_engine_filter_events()->command = on_command;
   node_get_engine_filter_events()->destroy = on_filter_destroy;
   node_get_engine_filter_events()->param_changed = on_param_changed;
   node_setup();

   engine_ports_setup();

   lilv_instance_activate(host->instance);  // create host_activate() and call it?

   // embed this in a function host_apply_preset (can we make host indep of engine and only
   // host, we could then pass loop with the call)
   if (strlen(host->preset_uri)) {
      pw_loop_invoke(pw_thread_loop_get_loop(node->engine_loop), host_on_preset, 0,
                     host->preset_uri, strlen(host->preset_uri), false, NULL);
   }

   if (host->start_ui)
      pw_loop_invoke(pw_thread_loop_get_loop(node->engine_loop), pluginui_on_start, 0, NULL,
                     0, false, NULL);
}
