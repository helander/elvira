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
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <stdio.h>
#include <unistd.h>

#include "handler.h"
#include "host.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Compilation conditions                       */
/* ========================================================================== */
#include <pipewire/version.h>
#define USE_UMP 0
#if PW_CHECK_VERSION(1,4,0)
#undef USE_UMP
#define USE_UMP 1
#endif

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */
#if USE_UMP
static const char *midi_input_format = "32 bit raw UMP";
#else
static const char *midi_input_format = "8 bit raw midi";
#endif

static Node the_node;

static struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = on_process,
    .destroy = on_destroy,
    .state_changed = on_state_changed,
    .param_changed = on_param_changed,
    /*
        .command = on_command,
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

static const char *audio_in_0 = "in.FL";
static const char *audio_in_1 = "in.FR";
static const char *audio_out_0 = "out.FL";
static const char *audio_out_1 = "out.FR";
static const char *midi_in_0 = "in.midi";
static const char *midi_out_0 = "out.midi";
//static const char *audio_channel_0 = "FL";
//static const char *audio_channel_1 = "FR";
/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
static void create_node_ports() {
   int audio_input_index = 0;
   int audio_output_index = 0;
   int midi_input_index = 0;
   int midi_output_index = 0;
   const char *port_name;
   //const char *audio_channel;
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
            switch (midi_input_index) {
               case 0:
                   port_name = midi_in_0;
                   break;
               default:
                   port_name = host_port->name;
            }
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, midi_input_format, PW_KEY_PORT_NAME,
                                  port_name, NULL),
                NULL, 0);
            node_port->type = NODE_CONTROL_INPUT;
            midi_input_index++;
            break;
         case HOST_ATOM_OUTPUT:
            switch (midi_output_index) {
               case 0:
                   port_name = midi_out_0;
                   break;
               default:
                   port_name = host_port->name;
            }
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi", PW_KEY_PORT_NAME,
                                  port_name, NULL),
                NULL, 0);
            node_port->type = NODE_CONTROL_OUTPUT;
            midi_output_index++;
            break;
         case HOST_AUDIO_INPUT:
            switch (audio_input_index) {
               case 0:
                   port_name = audio_in_0;
                   //audio_channel = audio_channel_0;
                   break;
               case 1:
                   port_name = audio_in_1;
                   //audio_channel = audio_channel_1;
                   break;
               default:
                   port_name = host_port->name;
                   //audio_channel = "";
            }
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME,
                                  port_name, /*"audio.channel", audio_channel,*/ NULL),
                NULL, 0);
            node_port->type = NODE_AUDIO_INPUT;
            audio_input_index++;
            break;
         case HOST_AUDIO_OUTPUT:
            switch (audio_output_index) {
               case 0:
                   port_name = audio_out_0;
                   //audio_channel = audio_channel_0;
                   break;
               case 1:
                   port_name = audio_out_1;
                   //audio_channel = audio_channel_1;
                   break;
               default:
                   port_name = host_port->name;
                   //audio_channel = "";
            }
            node_port->pwPort = pw_filter_add_port(
                node->filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME,
                                  port_name, /*"audio.channel", audio_channel,*/ NULL),
                NULL, 0);
            node_port->type = NODE_AUDIO_OUTPUT;
            audio_output_index++;
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

   struct pw_context *context =
       pw_context_new(pw_thread_loop_get_loop(runtime_primary_event_loop), NULL, 0);
   struct pw_core *core = pw_context_connect(context, NULL, 0);

   node->registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
   pw_registry_add_listener(node->registry, &registry_listener, &registry_events, node);

   node->gain = 1.0;
   node->previous_gain = node->gain;

   char sgain[20];
   sprintf(sgain,"%f",node->gain);

   char spid[20];
   sprintf(spid,"%d",getpid());

   struct pw_properties *props;
   props = pw_properties_new(PW_KEY_NODE_LATENCY, latency, PW_KEY_NODE_ALWAYS_PROCESS, "true", NULL);
   //pw_properties_set(props, "media.class", "Audio/Filter");
   pw_properties_set(props, "media.name", "");
   //pw_properties_set(props, "audio.channels", "2");
   //pw_properties_set(props, "audio.position", "FL,FR");
   pw_properties_set(props, "elvira.role", "instance");
   if (config_group)
      pw_properties_set(props, "elvira.group", config_group);
   if (config_step)
      pw_properties_set(props, "elvira.step", config_step);
   pw_properties_set(props, "elvira.plugin", config_plugin_uri);
   pw_properties_set(props, "elvira.preset", config_preset_uri);
   pw_properties_set(props, "elvira.host.info.base", host_info_base());
   pw_properties_set(props, "elvira.host.info.ports", host_info_ports());
   pw_properties_set(props, "elvira.host.info.params", host_info_params());
   //pw_properties_set(props, "elvira.autoconnect.audio", "true");
   //pw_properties_set(props, "elvira.autoconnect.midi", "true");
   pw_properties_set(props, "elvira.gain", sgain);
   pw_properties_set(props, "elvira.pid", spid);


   node->filter = pw_filter_new_simple(pw_thread_loop_get_loop(runtime_primary_event_loop),
                                       config_nodename, props, &filter_events, node);

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *volprops = spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
        SPA_PROP_volume, SPA_POD_Float(node->gain));
    pw_filter_update_params(node->filter, NULL, &volprops, 1);


   b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
   const struct spa_pod *params[1];

   params[0] = spa_process_latency_build(
       &b, SPA_PARAM_ProcessLatency, &SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC));

   if (pw_filter_connect(node->filter, PW_FILTER_FLAG_RT_PROCESS, params, 1) < 0) {
      pw_log_error("Node pw filter can't connect\n");
      return -1;
   }

   // Create required node ports
   create_node_ports();

   // All pw resources created, release the lock
   pw_thread_loop_unlock(runtime_primary_event_loop);
}


