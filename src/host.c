/*
 * ============================================================================
 *  File:       host.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      lv2 host related functions.
 *
 * ============================================================================
 */

#include "host.h"

#include <lilv/lilv.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/parameters/parameters.h>
#include <lv2/state/state.h>
#include <pipewire/pipewire.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "handler.h"
#include "node.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */
static Host the_host;
static char info[20000];

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
static const LV2_Feature buf_size_features[3] = {
    {LV2_BUF_SIZE__powerOf2BlockLength, NULL},
    {LV2_BUF_SIZE__fixedBlockLength, NULL},
    {LV2_BUF_SIZE__boundedBlockLength, NULL},
};

static LV2_Worker_Status my_schedule_work(LV2_Worker_Schedule_Handle handle, uint32_t size,
                                          const void *data) {
   pw_loop_invoke(pw_thread_loop_get_loop(runtime_worker_event_loop), on_host_worker, 0, data, size,
                  false, NULL);
   return LV2_WORKER_SUCCESS;
}

static void load_plugin() {
   LilvNode *uri = lilv_new_uri(constants.world, config_plugin_uri);
   LilvPlugin *plugin = NULL;
   if (uri != NULL) {
      const LilvPlugins *plugins = lilv_world_get_all_plugins(constants.world);
      plugin = (LilvPlugin *)lilv_plugins_get_by_uri(plugins, uri);
      lilv_node_free(uri);
      if (plugin == NULL) {
         pw_log_error("Host can't load plugin %s", config_plugin_uri);
      }
   } else {
      pw_log_error("Host plugin error in URI %s", config_plugin_uri);
   }
   host->lilvPlugin = plugin;
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
Host *host = &the_host;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int host_setup() {
   constants_init();
   load_plugin();
   {
      uint32_t n_features = 0;
      static const int32_t min_block_length = 1;
      static const int32_t max_block_length = 8192;
      static const int32_t seq_size = 32768;
      float fsample_rate = (float)config_samplerate;

      host->block_length = 1024;
      host->features[n_features++] = &constants.map_feature;
      host->features[n_features++] = &constants.unmap_feature;

      host->features[n_features++] = &buf_size_features[0];
      host->features[n_features++] = &buf_size_features[1];
      host->features[n_features++] = &buf_size_features[2];
      if (lilv_plugin_has_feature(host->lilvPlugin, constants.worker_schedule)) {
         host->work_schedule.handle = host;
         host->work_schedule.schedule_work = my_schedule_work;
         host->work_schedule_feature.URI = LV2_WORKER__schedule;
         host->work_schedule_feature.data = &host->work_schedule;
         host->features[n_features++] = &host->work_schedule_feature;
      }

      host->options[0] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__minBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &min_block_length};
      host->options[1] =
          (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                               0,
                               constants_map(constants, LV2_BUF_SIZE__maxBlockLength),
                               sizeof(int32_t),
                               constants.atom_Int,
                               &max_block_length};
      host->options[2] = (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                                              0,
                                              constants_map(constants, LV2_BUF_SIZE__sequenceSize),
                                              sizeof(int32_t),
                                              constants.atom_Int,
                                              &seq_size};
      host->options[3] = (LV2_Options_Option){
          LV2_OPTIONS_INSTANCE,
          0,
          constants_map(constants, "http://lv2plug.in/ns/ext/buf-size#nominalBlockLength"),
          sizeof(int32_t),
          constants.atom_Int,
          &host->block_length};
      host->options[4] = (LV2_Options_Option){LV2_OPTIONS_INSTANCE,
                                              0,
                                              constants_map(constants, LV2_PARAMETERS__sampleRate),
                                              sizeof(float),
                                              constants.atom_Float,
                                              &fsample_rate};
      host->options[5] = (LV2_Options_Option){LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL};

      host->options_feature.URI = LV2_OPTIONS__options;
      host->options_feature.data = host->options;
      host->features[n_features++] = &host->options_feature;

      host->instance = lilv_plugin_instantiate(host->lilvPlugin, config_samplerate, host->features);

      host->handle = lilv_instance_get_handle(host->instance);

      host->worker_data = NULL;
      if (host->instance != NULL) {
         if (lilv_plugin_has_extension_data(host->lilvPlugin, constants.worker_iface)) {
            host->iface = (const LV2_Worker_Interface *)lilv_instance_get_extension_data(
                host->instance, LV2_WORKER__interface);
         }
      }
   }

   host_ports_discover();

   spa_ringbuffer_init(&host->work_response_ring);

   return 0;
}

void host_ports_discover() {
   const LilvPlugin *plugin = host->lilvPlugin;
   set_init(&host->ports);
   int n_ports = lilv_plugin_get_num_ports(plugin);
   for (int n = 0; n < n_ports; n++) {
      HostPort *port = (HostPort *)calloc(1, sizeof(HostPort));
      port->index = n;

      port->lilvPort = lilv_plugin_get_port_by_index(plugin, n);
      strcpy(port->name, lilv_node_as_string(lilv_port_get_symbol(plugin, port->lilvPort)));
      if (lilv_port_is_a(plugin, port->lilvPort, constants.atom_AtomPort) &&
          lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         port->type = HOST_ATOM_INPUT;
         port->buffer = calloc(1, ATOM_BUFFER_SIZE);
         lilv_instance_connect_port(host->instance, port->index, port->buffer);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.atom_AtomPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         port->type = HOST_ATOM_OUTPUT;
         port->buffer = calloc(1, ATOM_BUFFER_SIZE);
         lilv_instance_connect_port(host->instance, port->index, port->buffer);

      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         LilvNode *lv2_default = lilv_new_uri(constants.world, LILV_NS_LV2 "default");
         LilvNode *default_val = lilv_port_get(plugin, port->lilvPort, lv2_default);
         if (default_val) {
            port->dfault = lilv_node_as_float(default_val);
         }
         port->type = HOST_CONTROL_INPUT;
         port->current = port->dfault;
         lilv_instance_connect_port(host->instance, port->index, &port->current);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         port->type = HOST_CONTROL_OUTPUT;
         lilv_instance_connect_port(host->instance, port->index, &port->current);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_AudioPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         port->type = HOST_AUDIO_INPUT;
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_AudioPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         port->type = HOST_AUDIO_OUTPUT;
      } else {
         free(port);
         port = NULL;
         pw_log_error("Host unsupported port type: port #%d (%s)", port->index, port->name);
      }
      set_add(&host->ports, port);
   }
}

char *host_info_base() {
   const LilvPlugin *p = host->lilvPlugin;
   strcpy(info, "{");

   strcat(info, "\"uri\":");
   strcat(info, "\"");
   strcat(info, lilv_node_as_string(lilv_plugin_get_uri(p)));
   strcat(info, "\"");
   strcat(info, ",\"name\":");
   strcat(info, "\"");
   strcat(info, lilv_node_as_string(lilv_plugin_get_name(p)));
   strcat(info, "\"");
   strcat(info, "}");

   return info;
}

char *host_info_ports() {
   const LilvPlugin *p = host->lilvPlugin;
   strcpy(info, "[");

   uint32_t num_ports = lilv_plugin_get_num_ports(p);
   for (uint32_t i = 0; i < num_ports; ++i) {
      const LilvPort *port = lilv_plugin_get_port_by_index(p, i);
      const LilvNode *symbol = lilv_port_get_symbol(p, port);
      const LilvNode *name = lilv_port_get_name(p, port);
      const char *ssymbol = lilv_node_as_string(symbol);
      const char *sname = lilv_node_as_string(name);

      if (i) strcat(info, ",");
      strcat(info, "{");
      sprintf(info + strlen(info), "\"index\":%d", i);
      sprintf(info + strlen(info), ",\"symbol\":\"%s\"", ssymbol);
      sprintf(info + strlen(info), ",\"name\":\"%s\"", sname);

      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_INPUT_PORT))) {
         sprintf(info + strlen(info), ",\"input\":true");
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_OUTPUT_PORT))) {
         sprintf(info + strlen(info), ",\"output\":true");
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_AUDIO_PORT))) {
         sprintf(info + strlen(info), ",\"audio\":true");
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_CONTROL_PORT))) {
         sprintf(info + strlen(info), ",\"control\":true");
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_ATOM_PORT))) {
         sprintf(info + strlen(info), ",\"atom\":true");
      }

      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_CONTROL_PORT))) {
         LilvNode *lv2_default = lilv_new_uri(constants.world, LILV_NS_LV2 "default");
         LilvNode *default_val = lilv_port_get(p, port, lv2_default);
         if (default_val) {
            sprintf(info + strlen(info), ",\"default\":%f", lilv_node_as_float(default_val));
         }
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_CONTROL_PORT))) {
         LilvNode *lv2_min = lilv_new_uri(constants.world, LILV_NS_LV2 "minimum");
         LilvNode *min_val = lilv_port_get(p, port, lv2_min);
         if (min_val) {
            sprintf(info + strlen(info), ",\"min\":%f", lilv_node_as_float(min_val));
         }
      }
      if (lilv_port_is_a(p, port, lilv_new_uri(constants.world, LILV_URI_CONTROL_PORT))) {
         LilvNode *lv2_max = lilv_new_uri(constants.world, LILV_NS_LV2 "maximum");
         LilvNode *max_val = lilv_port_get(p, port, lv2_max);
         if (max_val) {
            sprintf(info + strlen(info), ",\"max\":%f", lilv_node_as_float(max_val));
         }
      }

      LilvScalePoints *points = lilv_port_get_scale_points(p, port);
      if (points) {
         strcat(info, ",\"scale\":[");
         LILV_FOREACH(scale_points, i, points) {
            const LilvScalePoint *point = lilv_scale_points_get(points, i);
            if (i) strcat(info, ",");
            sprintf(info + strlen(info), "{\"%s\":%f}",
                    lilv_node_as_string(lilv_scale_point_get_label(point)),
                    lilv_node_as_float(lilv_scale_point_get_value(point)));
         }
         strcat(info, "]");
         lilv_scale_points_free(points);
      }

      // supported events
      // designations
      // groups
      // port properties
      strcat(info, "}");
   }

   strcat(info, "]");

   return info;
}
