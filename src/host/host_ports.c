#include "host_ports.h"

#include <spa/control/control.h>
#include <spa/control/ump-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "common/types.h"
#include "constants.h"
#include "utils/stb_ds.h"

void host_ports_discover(Engine *engine) {
   const LilvPlugin *plugin = engine->host.lilvPlugin;
   // char *enginename = engine->enginename;
   /// se till att detta görs vid init av EnginePorts   memset(dummyAudioInput, 0,
   /// sizeof(dummyAudioInput));
   engine->host.ports = NULL;
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
         lilv_instance_connect_port(engine->host.instance, port->index, port->buffer);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.atom_AtomPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         port->type = HOST_ATOM_OUTPUT;
         port->buffer = calloc(1, ATOM_BUFFER_SIZE);
         lilv_instance_connect_port(engine->host.instance, port->index, port->buffer);

      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_InputPort)) {
         LilvNode *lv2_default = lilv_new_uri(constants.world, LILV_NS_LV2 "default");
         LilvNode *default_val = lilv_port_get(plugin, port->lilvPort, lv2_default);
         if (default_val) {
            port->dfault = lilv_node_as_float(default_val);
         }
         port->type = HOST_CONTROL_INPUT;
         port->current = port->dfault;
         lilv_instance_connect_port(engine->host.instance, port->index, &port->current);
      } else if (lilv_port_is_a(plugin, port->lilvPort, constants.lv2_ControlPort) &&
                 lilv_port_is_a(plugin, port->lilvPort, constants.lv2_OutputPort)) {
         port->type = HOST_CONTROL_OUTPUT;
         lilv_instance_connect_port(engine->host.instance, port->index, &port->current);
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
      arrput(engine->host.ports, *port);
   }
}
