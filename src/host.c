/*
 * ============================================================================
 *  File:       host.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      .
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
         printf("\ncan't load plugin %s", config_plugin_uri);
      }
   } else {
      printf("\nerror in URI %s", config_plugin_uri);
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
         printf("\nUnsupported port type: port #%d (%s)", port->index, port->name);
      }
      set_add(&host->ports, port);
   }
}
