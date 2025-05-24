#include "host.h"

#include <lilv/lilv.h>
#include <spa/debug/types.h>
#include <spa/param/latency-utils.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/dynamic.h>
#include <spa/pod/parser.h>
#include <stdio.h>
#include <string.h>

#include "utils/constants.h"
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


int host_on_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
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

int host_setup(Engine *engine) {
   load_plugin(engine);
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


   ports_init(engine);

   spa_ringbuffer_init(&engine->host.work_response_ring);

   if (!strlen(engine->enginename))
      strcpy(engine->enginename,
             strdup(lilv_node_as_string(lilv_plugin_get_name(engine->host.lilvPlugin))));


   printf("\nStartup done for host [%s]", engine->enginename);
   fflush(stdout);
   return 0;
}

