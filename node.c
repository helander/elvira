#include "node.h"

#include <lilv/lilv.h>
#include <spa/debug/types.h>
#include <spa/param/latency-utils.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/dynamic.h>
#include <spa/pod/parser.h>
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "node_data.h"
#include "ports.h"
#include "program.h"
#include "ui.h"

const LV2_Feature buf_size_features[3] = {
    {LV2_BUF_SIZE__powerOf2BlockLength, NULL},
    {LV2_BUF_SIZE__fixedBlockLength, NULL},
    {LV2_BUF_SIZE__boundedBlockLength, NULL},
};

static LV2_Worker_Status xxx_worker_respond(LV2_Worker_Respond_Handle handle, const uint32_t size,
                                            const void *data) {
   // printf("\nrespond handle %lx",handle);fflush(stdout);
   struct node_data *node = (struct node_data *)handle;
   uint16_t len = size;
   if (size > MAX_WORK_RESPONSE_MESSAGE_SIZE) {
      fprintf(stderr, "Payload too large\n");
   } else {
      uint8_t temp[MAX_WORK_RESPONSE_MESSAGE_SIZE + sizeof(uint16_t)];
      memcpy(temp, &len, sizeof(uint16_t));
      memcpy(temp + sizeof(uint16_t), data, len);
      uint32_t total_len = len + sizeof(uint16_t);

      uint32_t write_index;
      spa_ringbuffer_get_write_index(&node->host.work_response_ring, &write_index);

      uint32_t ring_offset = write_index & (WORK_RESPONSE_RINGBUFFER_SIZE - 1);
      uint32_t space = WORK_RESPONSE_RINGBUFFER_SIZE - ring_offset;

      if (space >= total_len) {
         memcpy(node->host.work_response_buffer + ring_offset, temp, total_len);
      } else {
         // Wrap around
         memcpy(node->host.work_response_buffer + ring_offset, temp, space);
         memcpy(node->host.work_response_buffer, temp + space, total_len - space);
      }

      spa_ringbuffer_write_update(&node->host.work_response_ring, write_index + total_len);
   }

   return LV2_WORKER_SUCCESS;
}

static int on_worker(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                     void *user_data) {
   struct node_data *node = (struct node_data *)user_data;
   // printf("\nloop_worker");fflush(stdout);
   LV2_Worker_Status x =
       node->host.iface->work(node->host.handle, xxx_worker_respond, node, size, data);
   // printf("\nworker status %d",x);fflush(stdout);
   return x;
}

static LV2_Worker_Status my_schedule_work(LV2_Worker_Schedule_Handle handle, uint32_t size,
                                          const void *data) {
   struct node_data *node = (struct node_data *)handle;
   // printf("\nHost: schedule_work()");fflush(stdout);
   //  Fire execution of on_worker in loop thread
   pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_worker, 0, data, size, false, node);
   return LV2_WORKER_SUCCESS;
}

static void on_process(void *userdata, struct spa_io_position *position) {
   struct node_data *node = userdata;

   if (!node->pw.connected) {
      // Defer start of plugin UI until any node port is connected
      node->pw.connected = 1;
      if (node->host.start_ui)
         pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), pluginui_on_start, 0, NULL, 0,
                        false, node);
   }

   uint32_t n_samples = position->clock.duration;
   uint64_t frame = node->clock_time;
   float denom = (float)position->clock.rate.denom;
   node->clock_time += position->clock.duration;

   for (int n = 0; n < node->n_ports; n++) {
      struct port_data *port = &node->ports[n];
      if (port->pre_run) port->pre_run(port, node, frame, denom, (uint64_t)n_samples);
   }

   lilv_instance_run(node->host.instance, n_samples);
   /////
   {
      struct spa_ringbuffer *ring = &node->host.work_response_ring;
      uint8_t *buffer = node->host.work_response_buffer;
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
         node->host.iface->work_response(node->host.handle, msg_len, payload);

         read_index += sizeof(uint16_t) + msg_len;
         spa_ringbuffer_read_update(ring, read_index);
      }
   }
   /////

   for (int n = 0; n < node->n_ports; n++) {
      struct port_data *port = &node->ports[n];
      if (port->post_run) port->post_run(port, node);
   }
}

static void do_quit(void *userdata, int signal_number) {
   struct node_data *node = userdata;
   pw_thread_loop_signal(node->pw.loop, false);
}

static void load_plugin(struct node_data *node) {
   LilvNode *uri = lilv_new_uri(constants.world, node->plugin_uri);
   LilvPlugin *plugin = NULL;
   if (uri != NULL) {
      const LilvPlugins *plugins = lilv_world_get_all_plugins(constants.world);
      plugin = (LilvPlugin *)lilv_plugins_get_by_uri(plugins, uri);
      lilv_node_free(uri);
      if (plugin == NULL) {
         printf("\ncan't load plugin %s", node->plugin_uri);
      }
   } else {
      printf("\nerror in URI %s", node->plugin_uri);
   }
   node->host.lilvPlugin = plugin;
}

static int on_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                     void *user_data) {
   struct node_data *node = (struct node_data *)user_data;
   char *preset_uri = (char *)data;
   printf("\nincoming preset uri %lx [%s]  %lx", preset_uri, preset_uri, node);

   printf("\nselect [%s]", preset_uri);
   pthread_mutex_lock(&program_lock);

   if (strlen(preset_uri)) {
      printf("\nAttempt to apply preset %s.", preset_uri);
      fflush(stdout);
      node->host.lilv_preset = lilv_new_uri(constants.world, preset_uri);

      if (node->host.lilv_preset) {
         lilv_world_load_resource(constants.world, node->host.lilv_preset);
         LilvState *state =
             lilv_state_new_from_world(constants.world, &constants.map, node->host.lilv_preset);
         if (state) {
            // printf("\nSTATE: %s\n",lilv_state_to_string(constants.world,
            // &constants.map, &constants.unmap, state, "http://mystate",
            // NULL));fflush(stdout);
            LV2_Feature urid_feature = {
                .URI = LV2_URID__map,
                .data = &constants.map,
            };
            const LV2_Feature *features[] = {&urid_feature, NULL};

            lilv_state_restore(state, node->host.instance, NULL, NULL, 0, features);
         } else {
            printf("\nNo preset to load.");
            fflush(stdout);
         }
      } else {
         printf("\nNo preset specified.");
         fflush(stdout);
      }
   }
   lilv_instance_deactivate(node->host.instance);
   pthread_mutex_unlock(&program_lock);
}

static void on_command(void *data, const struct spa_command *command) {
   struct node_data *node = (struct node_data *)data;
   uint32_t id = SPA_NODE_COMMAND_ID(command);
   if (id == SPA_NODE_COMMAND_User) {
      uint8_t *p = (uint8_t *)command;
      char *command_string = p + 32;

      printf("\nCommand---[%s]", command_string);

      char uri[100];
      if (sscanf(command_string, "preset %s", uri) == 1) {
         pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_preset, 0, uri, strlen(uri) + 1,
                        false, node);
      } else {
         printf("\nUnknown command [%s]", command_string);
      }

   } else {
      printf("\ngot id %d (%s)", id, spa_debug_type_find_name(spa_type_node_command_id, id));
   }
   // dump("pod",&command->pod,command->pod.size+64);
   // print_command(&command->pod);

   fflush(stdout);
}

void on_param_changed(void *data, void *port_data, uint32_t id, const struct spa_pod *param) {
   printf("\nParam changed type %d  size %d", param->type, param->size);
   if (param->type == SPA_TYPE_Object) printf("\nobject");
   fflush(stdout);
}

static const struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
    .command = on_command,
    .param_changed = on_param_changed,
};

static int on_start(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                    void *user_data) {
   struct node_data *node = (struct node_data *)user_data;

   pthread_mutex_lock(&program_lock);
   load_plugin(node);

   char latency[50];

   sprintf(latency, "%d/%d", node->pw.latency_period, node->pw.samplerate);

   node->pw.connected = 0;

   spa_ringbuffer_init(&node->host.work_response_ring);

   if (!strlen(node->nodename))
      strcpy(node->nodename,
             strdup(lilv_node_as_string(lilv_plugin_get_name(node->host.lilvPlugin))));

   pw_loop_add_signal(pw_thread_loop_get_loop(node->pw.loop), SIGINT, do_quit, node);
   pw_loop_add_signal(pw_thread_loop_get_loop(node->pw.loop), SIGTERM, do_quit, node);

   node->pw.filter =
       pw_filter_new_simple(pw_thread_loop_get_loop(node->pw.loop), node->nodename,
                            pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                                              "Filter", PW_KEY_MEDIA_ROLE, "DSP", PW_KEY_MEDIA_NAME,
                                              "--", PW_KEY_NODE_LATENCY, latency, NULL),
                            &filter_events, node);

   ports_init(node);

   uint8_t buffer[1024];
   struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(node->pw.filter, PW_FILTER_FLAG_RT_PROCESS | PW_FILTER_FLAG_DRIVER, params,
                         1) < 0) {
      fprintf(stderr, "can't connect\n");
      pthread_mutex_unlock(&program_lock);
      return -1;
   }

   // send_string_param(node->pw.filter, NULL);

   {
      uint32_t n_features = 0;
      static const int32_t min_block_length = 1;
      static const int32_t max_block_length = 8192;
      static const int32_t seq_size = 32768;
      float fsample_rate = (float)node->pw.samplerate;

      node->host.block_length = 1024;
      node->host.features[n_features++] = &constants.map_feature;
      node->host.features[n_features++] = &constants.unmap_feature;

      node->host.features[n_features++] = &buf_size_features[0];
      node->host.features[n_features++] = &buf_size_features[1];
      node->host.features[n_features++] = &buf_size_features[2];
      if (lilv_plugin_has_feature(node->host.lilvPlugin, constants.worker_schedule)) {
         node->host.work_schedule.handle = node;
         node->host.work_schedule.schedule_work = my_schedule_work;
         node->host.work_schedule_feature.URI = LV2_WORKER__schedule;
         node->host.work_schedule_feature.data = &node->host.work_schedule;
         node->host.features[n_features++] = &node->host.work_schedule_feature;
      }

      node->host.options[0] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__minBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &min_block_length};
      node->host.options[1] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__maxBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &max_block_length};
      node->host.options[2] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__sequenceSize),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &seq_size};
      node->host.options[3] = (LV2_Options_Option){
          LV2_OPTIONS_INSTANCE,
          0,
          constants_map(constants, "http://lv2plug.in/ns/ext/buf-size#nominalBlockLength"),
          sizeof(int32_t),
          constants.atom_Int,
          &node->host.block_length};
      node->host.options[4] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_PARAMETERS__sampleRate),
                               sizeof(float),
                               constants.atom_Float,
                               &fsample_rate};
      node->host.options[5] = (LV2_Options_Option){LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL};

      node->host.options_feature.URI = LV2_OPTIONS__options;
      node->host.options_feature.data = node->host.options;
      node->host.features[n_features++] = &node->host.options_feature;

      node->host.instance =
          lilv_plugin_instantiate(node->host.lilvPlugin, node->pw.samplerate, node->host.features);

      node->host.handle = lilv_instance_get_handle(node->host.instance);

      node->host.worker_data = NULL;
      if (node->host.instance != NULL) {
         if (lilv_plugin_has_extension_data(node->host.lilvPlugin, constants.worker_iface)) {
            node->host.iface = (const LV2_Worker_Interface *)lilv_instance_get_extension_data(
                node->host.instance, LV2_WORKER__interface);
         }
      }
   }

   // setup port, i.e. create pw ports and some permanent port buffer
   // "connections"
   for (int n = 0; n < node->n_ports; n++) {
      struct port_data *port = &node->ports[n];
      if (port->setup) port->setup(port, node);
   }

   lilv_instance_activate(node->host.instance);
   pthread_mutex_unlock(&program_lock);

   pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_preset, 0, node->preset_uri,
                  sizeof(node->preset_uri), false, node);

   printf("\nStartup done.");
   fflush(stdout);
}

void node_set_name(struct node_data *node, const char *node_name) {
   strcpy(node->nodename, node_name);
}

void node_set_samplerate(struct node_data *node, int samplerate) {
   node->pw.samplerate = samplerate;
}

void node_set_latency(struct node_data *node, int period) { node->pw.latency_period = period; }

void node_init(struct node_data *node) {
   node->nodename[0] = 0;
   node->plugin_uri[0] = 0;
   node->preset_uri[0] = 0;
   node->host.lilv_preset = NULL;
   node->host.start_ui = 1;
   node->host.suil_instance = NULL;
   node->pw.loop = pw_thread_loop_new(node->nodename, NULL);
}

void node_start(struct node_data *node) {
   printf("\nnode start [%s] %lx   %lx", node->preset_uri, node->preset_uri, node);
   pw_thread_loop_start(node->pw.loop);
   pw_loop_invoke(pw_thread_loop_get_loop(node->pw.loop), on_start, 0, NULL, 0, false, node);
}

void node_set_plugin(struct node_data *node, const char *plugin_uri) {
   strcpy(node->plugin_uri, plugin_uri);
}

void node_set_preset(struct node_data *node, const char *preset_uri) {
   printf("\nset preset [%s] %lx   %lx", preset_uri, node->preset_uri, node);
   strcpy(node->preset_uri, preset_uri);
   printf("\nread preset [%s]", node->preset_uri);
}

void node_set_ui_show(struct node_data *node, int show_ui) { node->host.start_ui = show_ui; }
