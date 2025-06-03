#include "node.h"

#include <spa/param/latency-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "utils/stb_ds.h"
//#include "engine.h"
#include "types.h"

static 
struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
};

struct pw_filter_events *node_get_engine_filter_events() {
  return &filter_events;
}

static
void  create_node_ports(Engine *engine) {
   engine->node.ports = NULL;
   for (int n = 0; n < arrlen(engine->host.ports); n++) {
      HostPort *host_port = &engine->host.ports[n];
      NodePort *node_port = (NodePort *) calloc(1,sizeof(NodePort));
      node_port->index = host_port->index;
      switch(host_port->type) {
        case HOST_CONTROL_INPUT:
          /*
          node_port->pwPort = pw_filter_add_port(engine->node.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                          pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32", PW_KEY_PORT_NAME,
                                            host_port->name, NULL),
                          NULL, 0);
          node_port->type = NODE_CONTROL_INPUT;
          */
          free(node_port);
          node_port = NULL;
          break;
        case HOST_CONTROL_OUTPUT:
          /*
          node_port->pwPort = pw_filter_add_port( engine->node.filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                                pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32", PW_KEY_PORT_NAME, host_port->name, NULL),
                                NULL, 0);
          node_port->type = NODE_CONTROL_OUTPUT;
          */
          free(node_port);
          node_port = NULL;
          break;
        case HOST_ATOM_INPUT:
           node_port->pwPort = pw_filter_add_port(engine->node.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                              pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME, host_port->name, NULL),
                            NULL, 0);
           node_port->type = NODE_CONTROL_INPUT;
           break;
        case HOST_ATOM_OUTPUT:
           node_port->pwPort = pw_filter_add_port(engine->node.filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
       pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME, host_port->name, NULL),
       NULL, 0);
           node_port->type = NODE_CONTROL_OUTPUT;
          break;
        case HOST_AUDIO_INPUT:
           node_port->pwPort = pw_filter_add_port(engine->node.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                                     pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                                       PW_KEY_PORT_NAME, host_port->name, NULL),
                                     NULL, 0);
           node_port->type = NODE_AUDIO_INPUT;
          break;
        case HOST_AUDIO_OUTPUT:
           node_port->pwPort = pw_filter_add_port(engine->node.filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                          pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                            PW_KEY_PORT_NAME, host_port->name, NULL),
                          NULL, 0);
           node_port->type = NODE_AUDIO_OUTPUT;
          break;
        default:
          fprintf(stderr,"\nUnknown host port type %d",host_port->type);fflush(stderr);
          free(node_port);
          node_port = NULL;
      }
      if (node_port) arrput(engine->node.ports, *node_port);
   }
}



int node_setup(Engine *engine) {
   char latency[50];

   sprintf(latency, "%d/%d", engine->node.latency_period, engine->node.samplerate);

   // Create pw engine loop resources. Lock the engine loop
   pw_thread_loop_lock(engine->node.engine_loop);



   struct pw_properties *props;
   props = pw_properties_new(PW_KEY_NODE_LATENCY, latency, NULL);
   pw_properties_set(props, "elvira.role", "engine");
   pw_properties_set(props, "elvira.set", engine->setname);
   pw_properties_set(props, "elvira.plugin", engine->plugin_uri);
   pw_properties_set(props, "elvira.preset", engine->preset_uri);
   pw_properties_set(props, PW_KEY_MEDIA_NAME, engine->setname);

   engine->node.filter = pw_filter_new_simple(pw_thread_loop_get_loop(engine->node.engine_loop), engine->enginename, props, &filter_events, engine);


   uint8_t buffer[1024];
   struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(engine->node.filter, PW_FILTER_FLAG_RT_PROCESS ,
                         params, 1) < 0) {
      fprintf(stderr, "can't connect\n");
      return -1;
   }

   // Create required node ports
   create_node_ports(engine);

   // All pw resources created, release the lock
   pw_thread_loop_unlock(engine->node.engine_loop);

}
