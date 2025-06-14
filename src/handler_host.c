/*
 * ============================================================================
 *  File:       handler_host.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handlers related to lv2 host functions.
 *      
 * ============================================================================
 */

#include <string.h>
#include <lilv/lilv.h>
#include <lv2/state/state.h>

#include "constants.h"
#include "handler.h"
#include "host.h"
#include "node.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
const char* get_preset_tail(const char* url) {
    const char *last_slash = strrchr(url, '/');
    if (!last_slash) {
        return ""; // No slash at all
    }

    // Check if "preset" comes just before the last slash
    size_t len_before = last_slash - url;
    if (len_before >= 6 && strncmp(last_slash - 6, "preset", 6) == 0) {
        return *(last_slash + 1) ? last_slash + 1 : ""; // Return after slash if anything exists
    }

    return ""; // Doesn't match "preset/" rule
}

static void apply_preset(char *preset_uri) {
   if (strlen(preset_uri)) {
      host->lilv_preset = lilv_new_uri(constants.world, preset_uri);

      if (host->lilv_preset) {
         lilv_world_load_resource(constants.world, host->lilv_preset);
         LilvState *state =
             lilv_state_new_from_world(constants.world, &constants.map, host->lilv_preset);
         if (state) {
            LV2_Feature urid_feature = {
                .URI = LV2_URID__map,
                .data = &constants.map,
            };
            const LV2_Feature *features[] = {&urid_feature, NULL};

            lilv_state_restore(state, host->instance, NULL, NULL, 0, features);
            pw_log_info("Preset with URI: %s applied", preset_uri);

            pw_thread_loop_lock(runtime_primary_event_loop);

            struct spa_dict_item items[2];
            items[0] = SPA_DICT_ITEM_INIT("elvira.preset", preset_uri);
            items[1] = SPA_DICT_ITEM_INIT("media.name", get_preset_tail(preset_uri));
            pw_filter_update_properties(node->filter, NULL, &SPA_DICT_INIT(items, 2));



            pw_thread_loop_unlock(runtime_primary_event_loop);

         } else {
            pw_log_error("No preset to load.");
         }
      } else {
         pw_log_error("No preset specified.");
      }
   }
}

static LV2_Worker_Status the_worker_respond(LV2_Worker_Respond_Handle handle, const uint32_t size,
                                            const void *data) {
   uint16_t len = size;
   if (size > MAX_WORK_RESPONSE_MESSAGE_SIZE) {
      pw_log_error("Payload too large");
   } else {
      uint8_t temp[MAX_WORK_RESPONSE_MESSAGE_SIZE + sizeof(uint16_t)];
      memcpy(temp, &len, sizeof(uint16_t));
      memcpy(temp + sizeof(uint16_t), data, len);
      uint32_t total_len = len + sizeof(uint16_t);

      uint32_t write_index;
      spa_ringbuffer_get_write_index(&host->work_response_ring, &write_index);

      uint32_t ring_offset = write_index & (WORK_RESPONSE_RINGBUFFER_SIZE - 1);
      uint32_t space = WORK_RESPONSE_RINGBUFFER_SIZE - ring_offset;

      if (space >= total_len) {
         memcpy(host->work_response_buffer + ring_offset, temp, total_len);
      } else {
         memcpy(host->work_response_buffer + ring_offset, temp, space);
         memcpy(host->work_response_buffer, temp + space, total_len - space);
      }

      spa_ringbuffer_write_update(&host->work_response_ring, write_index + total_len);
   }

   return LV2_WORKER_SUCCESS;
}

static const void *port_value(const char *port_symbol, void *user_data, uint32_t *size,
                              uint32_t *type) {
   HostPort *port;
   SET_FOR_EACH(HostPort *, port, &host->ports) {
      if (strcmp(port_symbol, port->name)) continue;
      *size = sizeof(float);
      *type = constants.forge.Float;
      return &port->current;
   }
   *type = 0;
   *size = 0;
   return NULL;
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int on_host_worker(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                   void *user_data) {
   LV2_Worker_Status status = host->iface->work(host->handle, the_worker_respond, host, size, data);
   return status;
}

int on_host_preset(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                   void *user_data) {
   char *preset_uri = (char *)data;

   apply_preset(preset_uri);
   return 0;
}

int on_host_save(struct spa_loop *loop, bool async, uint32_t seq, const void *data, size_t size,
                 void *user_data) {
   char *preset_name = (char *)data;
   if (strlen(preset_name)) {
      const LV2_Feature *features[] = {&constants.map_feature, &constants.unmap_feature, NULL};
      char preset_dir[200];
      sprintf(preset_dir, "%s/.lv2/%s", getenv("HOME"), preset_name);

      // Create the preset state
      LilvState *state = lilv_state_new_from_instance(
          host->lilvPlugin, host->instance, &constants.map, "/tmp/elvira", preset_dir, preset_dir,
          preset_dir, port_value, host, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, features);

      if (!state) {
         pw_log_error("Failed to create the preset state");
         return -1;
      }

      char preset_uri[200];

      sprintf(preset_uri, "%s/preset/%s", lilv_node_as_uri(lilv_plugin_get_uri(host->lilvPlugin)),
              preset_name);

      // Save the created preset on filesystem
      lilv_state_save(constants.world, &constants.map, &constants.unmap, state, preset_uri,
                      preset_dir, "state.ttl");

      lilv_state_free(state);

      // Various methods has been tested to make the newly created preset be available without
      // having to restart the program. The methods below works, but is a bit "brutal". Any other
      // methods that should be tested?
      lilv_world_load_all(constants.world);
     
      pw_log_debug("Preset saved to %s with URI: %s", preset_dir, preset_uri);

      apply_preset(preset_uri);
   }
   return 0;
}
