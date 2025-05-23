#include "engine.h"

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
#include "engine_data.h"
#include "ports.h"
//#include "program.h"
#include "ui.h"
//#include "group.h"

const LV2_Feature buf_size_features[3] = {
    {LV2_BUF_SIZE__powerOf2BlockLength, NULL},
    {LV2_BUF_SIZE__fixedBlockLength, NULL},
    {LV2_BUF_SIZE__boundedBlockLength, NULL},
};



static LV2_Worker_Status xxx_worker_respond(LV2_Worker_Respond_Handle handle, const uint32_t size,
                                            const void *data) {
   Engine *engine = (Engine *)handle;
   uint16_t len = size;
   if (size > MAX_WORK_RESPONSE_MESSAGE_SIZE) {
      fprintf(stderr, "Payload too large\n");
   } else {
      uint8_t temp[MAX_WORK_RESPONSE_MESSAGE_SIZE + sizeof(uint16_t)];
      memcpy(temp, &len, sizeof(uint16_t));
      memcpy(temp + sizeof(uint16_t), data, len);
      uint32_t total_len = len + sizeof(uint16_t);

      uint32_t write_index;
      spa_ringbuffer_get_write_index(&engine->host.work_response_ring, &write_index);

      uint32_t ring_offset = write_index & (WORK_RESPONSE_RINGBUFFER_SIZE - 1);
      uint32_t space = WORK_RESPONSE_RINGBUFFER_SIZE - ring_offset;

      if (space >= total_len) {
         memcpy(engine->host.work_response_buffer + ring_offset, temp, total_len);
      } else {
         // Wrap around
         memcpy(engine->host.work_response_buffer + ring_offset, temp, space);
         memcpy(engine->host.work_response_buffer, temp + space, total_len - space);
      }

      spa_ringbuffer_write_update(&engine->host.work_response_ring, write_index + total_len);
   }

   return LV2_WORKER_SUCCESS;
}

static int on_worker(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                     void *user_data) {
   Engine *engine = (Engine *)user_data;
   // printf("\nloop_worker");fflush(stdout);
   LV2_Worker_Status x =
       engine->host.iface->work(engine->host.handle, xxx_worker_respond, engine, size, data);
   // printf("\nworker status %d",x);fflush(stdout);
   return x;
}

static LV2_Worker_Status my_schedule_work(LV2_Worker_Schedule_Handle handle, uint32_t size,
                                          const void *data) {
   Engine *engine = (Engine *)handle;
   // printf("\nHost: schedule_work()");fflush(stdout);
   //  Fire execution of on_worker in loop thread
   pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.worker_loop), on_worker, 0, data, size, false,
                  engine);
   return LV2_WORKER_SUCCESS;
}

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

static void load_plugin(Engine *engine) {
   LilvNode *uri = lilv_new_uri(constants.world, engine->plugin_uri);
   LilvPlugin *plugin = NULL;
   if (uri != NULL) {
      const LilvPlugins *plugins = lilv_world_get_all_plugins(constants.world);
      plugin = (LilvPlugin *)lilv_plugins_get_by_uri(plugins, uri);
      lilv_node_free(uri);
      if (plugin == NULL) {
         printf("\ncan't load plugin %s", engine->plugin_uri);
      }
   } else {
      printf("\nerror in URI %s", engine->plugin_uri);
   }
   engine->host.lilvPlugin = plugin;
}


static int on_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                     void *user_data) {
   Engine *engine = (Engine *)user_data;
   char *preset_uri = (char *)data;

   if (strlen(preset_uri)) {
      printf("\nAttempt to apply preset %s.", preset_uri);
      fflush(stdout);
      engine->host.lilv_preset = lilv_new_uri(constants.world, preset_uri);

      if (engine->host.lilv_preset) {
         lilv_world_load_resource(constants.world, engine->host.lilv_preset);
         LilvState *state =
             lilv_state_new_from_world(constants.world, &constants.map, engine->host.lilv_preset);
         if (state) {
            // printf("\nSTATE: %s\n",lilv_state_to_string(constants.world,
            // &constants.map, &constants.unmap, state, "http://mystate",
            // NULL));fflush(stdout);
            LV2_Feature urid_feature = {
                .URI = LV2_URID__map,
                .data = &constants.map,
            };
            const LV2_Feature *features[] = {&urid_feature, NULL};

            lilv_state_restore(state, engine->host.instance, NULL, NULL, 0, features);
         } else {
            printf("\nNo preset to load.");
            fflush(stdout);
         }
      } else {
         printf("\nNo preset specified.");
         fflush(stdout);
      }
   }
   return 0;
}

static void on_command(void *data, const struct spa_command *command) {
   Engine *engine = (Engine *)data;
   printf("\n oncom start  [%s] engine %lx  filter %lx", engine->enginename, engine, engine->pw.filter);
   fflush(stdout);
   uint32_t id = SPA_NODE_COMMAND_ID(command);
   if (id == SPA_NODE_COMMAND_User) {
      uint8_t *p = (uint8_t *)command;
      char *command_string = p + 32;

      printf("\nCommand---[%s]", command_string);

      char uri[100];
      if (sscanf(command_string, "preset %s", uri) == 1) {
         pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.master_loop), on_preset, 0, uri, strlen(uri) + 1,
                        false, engine);
      } else {
         printf("\nUnknown command [%s]", command_string);
      }

   } else {
      printf("\ngot id %d (%s)", id, spa_debug_type_find_name(spa_type_node_command_id, id));
   }
   // dump("pod",&command->pod,command->pod.size+64);
   // print_command(&command->pod);
   printf("\n oncom end  [%s] engine %lx  filter %lx", engine->enginename, engine, engine->pw.filter);
   fflush(stdout);

   fflush(stdout);
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

static const struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
    .command = on_command,
    .destroy = on_filter_destroy,
    .param_changed = on_param_changed,
};

static int run_engine(Engine *engine) {
   load_plugin(engine);
   char latency[50];

   sprintf(latency, "%d/%d", engine->pw.latency_period, engine->pw.samplerate);

   engine->pw.connected = 0;

   spa_ringbuffer_init(&engine->host.work_response_ring);

   if (!strlen(engine->enginename))
      strcpy(engine->enginename,
             strdup(lilv_node_as_string(lilv_plugin_get_name(engine->host.lilvPlugin))));

   engine->pw.filter =
       pw_filter_new_simple(pw_thread_loop_get_loop(engine->pw.engine_loop), engine->enginename,
                            pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                                              "Filter", PW_KEY_MEDIA_ROLE, "DSP", PW_KEY_MEDIA_NAME,
                                              engine->groupname, PW_KEY_NODE_LATENCY, latency, NULL),
                            &filter_events, engine);

   ports_init(engine);

   uint8_t buffer[1024];
   struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(engine->pw.filter, PW_FILTER_FLAG_RT_PROCESS | PW_FILTER_FLAG_DRIVER, params,
                         1) < 0) {
      fprintf(stderr, "can't connect\n");
      // pthread_mutex_unlock(&program_lock);
      return -1;
   }

   {
      uint32_t n_features = 0;
      static const int32_t min_block_length = 1;
      static const int32_t max_block_length = 8192;
      static const int32_t seq_size = 32768;
      float fsample_rate = (float)engine->pw.samplerate;

      engine->host.block_length = 1024;
      engine->host.features[n_features++] = &constants.map_feature;
      engine->host.features[n_features++] = &constants.unmap_feature;

      engine->host.features[n_features++] = &buf_size_features[0];
      engine->host.features[n_features++] = &buf_size_features[1];
      engine->host.features[n_features++] = &buf_size_features[2];
      if (lilv_plugin_has_feature(engine->host.lilvPlugin, constants.worker_schedule)) {
         engine->host.work_schedule.handle = engine;
         engine->host.work_schedule.schedule_work = my_schedule_work;
         engine->host.work_schedule_feature.URI = LV2_WORKER__schedule;
         engine->host.work_schedule_feature.data = &engine->host.work_schedule;
         engine->host.features[n_features++] = &engine->host.work_schedule_feature;
      }

      engine->host.options[0] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__minBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &min_block_length};
      engine->host.options[1] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__maxBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &max_block_length};
      engine->host.options[2] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__sequenceSize),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &seq_size};
      engine->host.options[3] = (LV2_Options_Option){
          LV2_OPTIONS_INSTANCE,
          0,
          constants_map(constants, "http://lv2plug.in/ns/ext/buf-size#nominalBlockLength"),
          sizeof(int32_t),
          constants.atom_Int,
          &engine->host.block_length};
      engine->host.options[4] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_PARAMETERS__sampleRate),
                               sizeof(float),
                               constants.atom_Float,
                               &fsample_rate};
      engine->host.options[5] = (LV2_Options_Option){LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL};

      engine->host.options_feature.URI = LV2_OPTIONS__options;
      engine->host.options_feature.data = engine->host.options;
      engine->host.features[n_features++] = &engine->host.options_feature;

      engine->host.instance =
          lilv_plugin_instantiate(engine->host.lilvPlugin, engine->pw.samplerate, engine->host.features);

      engine->host.handle = lilv_instance_get_handle(engine->host.instance);

      engine->host.worker_data = NULL;
      if (engine->host.instance != NULL) {
         if (lilv_plugin_has_extension_data(engine->host.lilvPlugin, constants.worker_iface)) {
            engine->host.iface = (const LV2_Worker_Interface *)lilv_instance_get_extension_data(
                engine->host.instance, LV2_WORKER__interface);
         }
      }
   }


   for (int n = 0; n < engine->n_ports; n++) {
      struct port_data *port = &engine->ports[n];
      if (port->setup) port->setup(port, engine);
   }

   lilv_instance_activate(engine->host.instance);

   pw_thread_loop_start(engine->pw.engine_loop);
   if (engine->pw.worker_loop != engine->pw.engine_loop) {
      pw_thread_loop_start(engine->pw.worker_loop);
   }

   pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.master_loop), on_preset, 0, engine->preset_uri,
                  sizeof(engine->preset_uri), false, engine);

   printf("\nStartup done for engine [%s]", engine->enginename);
   fflush(stdout);
}

void engine_defaults(Engine *engine) {
   engine->groupname[0] = 0;
   engine->enginename[0] = 0;
   engine->plugin_uri[0] = 0;
   engine->preset_uri[0] = 0;
   engine->host.start_ui = true;
   engine->host.worker_loop = false;
   engine->host.engine_worker_loop = false;
   engine->pw.samplerate = 48000;
   engine->pw.latency_period = 256;
}

#if 0
void node_destroy(struct node_data *node) {
   printf("\nTermination of node [%s]", node->nodename);
   fflush(stdout);
   int separate_worker_loop = node->pw.worker_loop != node->pw.node_loop;
   pw_thread_loop_destroy(node->pw.node_loop);
   if (separate_worker_loop) {
      pw_thread_loop_destroy(node->pw.worker_loop);
   }

   node->pw.node_loop = NULL;
   node->pw.worker_loop = NULL;
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
   engine->pw.worker_loop = engine->pw.engine_loop;

   printf("\nStarting engine %s in group %s",engine->enginename, engine->groupname);fflush(stdout);
   run_engine(engine);

}

