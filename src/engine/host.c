#include "host.h"

#include <lilv/lilv.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/parameters/parameters.h>
#include <lv2/state/state.h>
#include <pipewire/pipewire.h>
#include <stdint.h>
#include <stdio.h>

#include "constants.h"
#include "engine_data.h"
#include "ports.h"

const LV2_Feature buf_size_features[3] = {
    {LV2_BUF_SIZE__powerOf2BlockLength, NULL},
    {LV2_BUF_SIZE__fixedBlockLength, NULL},
    {LV2_BUF_SIZE__boundedBlockLength, NULL},
};

static LV2_Worker_Status the_worker_respond(LV2_Worker_Respond_Handle handle, const uint32_t size,
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
   LV2_Worker_Status status =
       engine->host.iface->work(engine->host.handle, the_worker_respond, engine, size, data);
   return status;
}

static LV2_Worker_Status my_schedule_work(LV2_Worker_Schedule_Handle handle, uint32_t size,
                                          const void *data) {
   Engine *engine = (Engine *)handle;
   pw_loop_invoke(pw_thread_loop_get_loop(engine->pw.engine_loop), on_worker, 0, data, size, false,
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
      engine->host.lilv_preset = lilv_new_uri(constants.world, preset_uri);

      if (engine->host.lilv_preset) {
         lilv_world_load_resource(constants.world, engine->host.lilv_preset);
         LilvState *state =
             lilv_state_new_from_world(constants.world, &constants.map, engine->host.lilv_preset);
         if (state) {
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

static float xyz = 25.3;

static const void *get_pott_value(const char *port_symbol, void *user_data, uint32_t *size,
                                  uint32_t *type) {
   Engine *const engine = (Engine *)user_data;
   *size = sizeof(float);
   *type = constants.forge.Float;
   return &xyz;
}

int host_on_save(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                 void *user_data) {
   Engine *engine = (Engine *)user_data;
   char *preset_name = (char *)data;

   if (strlen(preset_name)) {
      const LV2_Feature *features[] = {&constants.map_feature, &constants.unmap_feature, NULL};

      // Create the preset state
      LilvState *state = lilv_state_new_from_instance(
          engine->host.lilvPlugin, engine->host.instance, &constants.map, "/tmp/elvira", NULL, NULL,
          NULL, get_pott_value, engine, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, features);

      if (!state) {
         fprintf(stderr, "\nFailed to create the preset state");
         return -1;
      }

      // Save the created preset on filesystem
      char preset_uri[200];
      char preset_dir[200];
      char preset_ttl[50];
      char preset_ttl_url[200];

      sprintf(preset_uri, "%s/preset/%s",
              lilv_node_as_uri(lilv_plugin_get_uri(engine->host.lilvPlugin)), preset_name);
      sprintf(preset_dir, "/home/soundcan/.lv2/%s", preset_name);
      sprintf(preset_ttl, "%s.ttl", preset_name);
      sprintf(preset_ttl_url, "file://%s/%s", preset_dir, preset_ttl);

      lilv_state_save(constants.world, &constants.map, &constants.unmap, state, preset_uri,
                      preset_dir, preset_ttl);

      lilv_state_free(state);

      // Various methods has been tested to make the newly created preset be available without
      // having to restart the program. The methods below works, but is a bit "brutal". Any other
      // methods that should be tested?
      lilv_world_load_all(constants.world);

      printf("Preset saved to %s/%s with URI: %s\n", preset_dir, preset_ttl, preset_uri);
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

      engine->host.instance = lilv_plugin_instantiate(engine->host.lilvPlugin,
                                                      engine->pw.samplerate, engine->host.features);

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
