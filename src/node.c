/*
 * ============================================================================
 *  File:       node.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      pipewire node related functions.
 *      
 * ============================================================================
 */

#include "node.h"

#include <spa/param/latency-utils.h>
#include <spa/pod/builder.h>
#include <stdio.h>

#include "handler.h"
#include "host.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */
static Node the_node;

static struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS, .process = on_process, .param_changed = on_param_changed,
    .command = on_command,    .destroy = on_destroy,
    .state_changed=on_state_changed,
    /*
        .io_changed=on_io_changed,
        .add_buffer=on_add_buffer,
        .remove_buffer=on_remove_buffer,
        .drained=on_drained,
    */
};

static struct spa_hook registry_listener;

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = on_registry_global,
};

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
static void create_node_ports() {
   set_init(&node->ports);
   HostPort *host_port;
   SET_FOR_EACH(HostPort *, host_port, &host->ports) {
      NodePort *node_port = (NodePort *)calloc(1, sizeof(NodePort));
      node_port->index = host_port->index;
      switch (host_port->type) {
         case HOST_CONTROL_INPUT:
            /*
            node_port->pwPort = pw_filter_add_port(node->filter, PW_DIRECTION_INPUT,
            PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32",
            PW_KEY_PORT_NAME, host_port->name, NULL), NULL, 0); node_port->type =
            NODE_CONTROL_INPUT;
            */
            free(node_port);
            node_port = NULL;
            break;
         case HOST_CONTROL_OUTPUT:
            /*
            node_port->pwPort = pw_filter_add_port( node->filter, PW_DIRECTION_OUTPUT,
            PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32",
            PW_KEY_PORT_NAME, host_port->name, NULL), NULL, 0); node_port->type =
            NODE_CONTROL_OUTPUT;
            */
            free(node_port);
            node_port = NULL;
            break;
         case HOST_ATOM_INPUT:
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME,
                                  host_port->name, NULL),
                NULL, 0);
            node_port->type = NODE_CONTROL_INPUT;
            break;
         case HOST_ATOM_OUTPUT:
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME,
                                  host_port->name, NULL),
                NULL, 0);
            node_port->type = NODE_CONTROL_OUTPUT;
            break;
         case HOST_AUDIO_INPUT:
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME,
                                  host_port->name, NULL),
                NULL, 0);
            node_port->type = NODE_AUDIO_INPUT;
            break;
         case HOST_AUDIO_OUTPUT:
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME,
                                  host_port->name, NULL),
                NULL, 0);
            node_port->type = NODE_AUDIO_OUTPUT;
            break;
         default:
            pw_log_error("Unknown host port type %d", host_port->type);
            free(node_port);
            node_port = NULL;
      }
      if (node_port) {
         set_add(&node->ports, node_port);
      }
   }
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
Node *node = &the_node;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int node_setup() {
   char latency[50];

   sprintf(latency, "%d/%d", config_latency_period, config_samplerate);

   // Create pw resources. Lock the  loop
   pw_thread_loop_lock(runtime_primary_event_loop);

   struct pw_context *context = pw_context_new(pw_thread_loop_get_loop(runtime_primary_event_loop), NULL, 0);
   struct pw_core *core = pw_context_connect(context, NULL, 0);

   node->registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
   pw_registry_add_listener(node->registry, &registry_listener,
                             &registry_events, node);


   struct pw_properties *props;
   props = pw_properties_new(PW_KEY_NODE_LATENCY, latency, NULL);
   pw_properties_set(props, "elvira.role", "engine");
   pw_properties_set(props, "elvira.plugin", config_plugin_uri);
   pw_properties_set(props, "elvira.preset", config_preset_uri);

   node->filter = pw_filter_new_simple(pw_thread_loop_get_loop(runtime_primary_event_loop),
                                       config_nodename, props, &filter_events, node);

   uint8_t buffer[1024];
   struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(node->filter, PW_FILTER_FLAG_RT_PROCESS, params, 1) < 0) {
      pw_log_error( "Node pw filter can't connect\n");
      return -1;
   }

   // Create required node ports
   create_node_ports();

   // All pw resources created, release the lock
   pw_thread_loop_unlock(runtime_primary_event_loop);
}
