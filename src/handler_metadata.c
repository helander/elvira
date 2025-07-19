
/*
 * ============================================================================
 *  File:       handler_metadata.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for metadata changes.
 *
 * ============================================================================
 */
#include <lilv/lilv.h>
#include <lv2/patch/patch.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>


#include "constants.h"
#include "handler.h"
#include "runtime.h"
#include "node.h"
#include "ports.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int on_metadata_property(void *object,
                                 uint32_t subject,
                                 const char *key,
                                 const char *type,
                                 const char *value)
{
    Node *node = (Node *) object;
    pw_log_debug("Metadata update: node=%u  key=%s  value=%s  subject %u", node->node_id, key, value, subject);
    if (subject == node->node_id) {
      pw_log_debug("Subject match:  key=%s  value=%s", key, value);
      const char *control_prefix = "control.in.";
      if (!strcmp(key,"use_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_preset, 0, value, strlen(value) + 1, false, NULL);
      } else if (!strcmp(key,"save_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_save, 0, value, strlen(value) + 1, false, NULL);
      } else if (!strncmp(key,control_prefix,strlen(control_prefix))) {
         float float_value = atof(value);
         const char *port_index_suffix = &key[strlen(control_prefix)];
         int port_index = atoi(port_index_suffix);
         ports_write(NULL, port_index, sizeof(float), 0U, &float_value);
      } else {
          LilvNode* param_uri = lilv_new_uri(constants.world, key);
          LilvNode* rdf_type = lilv_new_uri(constants.world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
          LilvNode* lv2_parameter = lilv_new_uri(constants.world, "http://lv2plug.in/ns/lv2core#Parameter");
          LilvNode* midi_parameter = lilv_new_uri(constants.world, "http://helander.network/lv2/elvira#MidiParameter");

          bool param_exists = lilv_world_ask(constants.world, param_uri, rdf_type, lv2_parameter);
          bool midi_param_exists = lilv_world_ask(constants.world, param_uri, rdf_type, midi_parameter);

          if (midi_param_exists) {
              pw_log_debug("Midi Parameter:  key=%s  value=%s", key, value);

              LilvNode* atom_AtomPort = lilv_new_uri(constants.world, LV2_ATOM__AtomPort);
              LilvNode* atom_supports = lilv_new_uri(constants.world, LV2_ATOM__supports);
              //LilvNode* patch_Message = lilv_new_uri(constants.world, LV2_PATCH__Message);

              const LilvPlugin *plugin = host->lilvPlugin;

              int port_index = -1;
              uint32_t num_ports = lilv_plugin_get_num_ports(plugin);
              for (uint32_t i = 0; i < num_ports && port_index == -1; ++i) {
                 const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
                 if (lilv_port_is_a(plugin, port, atom_AtomPort) /*&&
                     lilv_port_has_property(plugin, port, atom_supports)*/) {

                    // Get values of atom:supports
                    const LilvNodes* values = lilv_port_get_value(plugin, port, atom_supports);
                    LILV_FOREACH(nodes, j, values) {
                        const LilvNode* val = lilv_nodes_get(values, j);
                        //if (lilv_node_equals(val, patch_Message)) {
                           port_index = i;
                        //}
                    }
                 }
              }

              lilv_node_free(atom_AtomPort);
              lilv_node_free(atom_supports);
              //lilv_node_free(patch_Message);

              if (port_index == -1) {
                pw_log_error("Found no port that supports atoms and midi messages");
              } else {
                pw_log_debug("Midi parameter using port %d", port_index);
                int cc,val;
                sscanf(key, "midicc.%d",&cc);
                sscanf(value, "%d",&val);
                pw_log_debug("Midi parameter using port %d cc %d  val %d", port_index,cc,val);

                LV2_Atom_Forge forge;
                uint8_t buffer[1000];
                LV2_Atom_Forge_Frame frame;

                int channel = 0;
                uint8_t msg[3] = {
                    (uint8_t)(0xB0 | (channel & 0x0F)),
                    cc,
                    val
                };

                LV2_URID midi_event = constants_map(constants, LV2_MIDI__MidiEvent);
                lv2_atom_forge_init(&forge, &constants.map);

                lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));
                lv2_atom_forge_atom(&forge, 3, midi_event);
                lv2_atom_forge_raw(&forge, msg, 3);

                lv2_atom_forge_pop(&forge, &frame);

                ports_write(NULL, port_index, ((LV2_Atom*)buffer)->size + sizeof(LV2_Atom), constants.atom_eventTransfer, buffer);

              }

          }

          if (param_exists) {
              pw_log_debug("Parameter:  key=%s  value=%s", key, value);

              LilvNode* atom_AtomPort = lilv_new_uri(constants.world, LV2_ATOM__AtomPort);
              LilvNode* atom_supports = lilv_new_uri(constants.world, LV2_ATOM__supports);
              LilvNode* patch_Message = lilv_new_uri(constants.world, LV2_PATCH__Message);

              const LilvPlugin *plugin = host->lilvPlugin;

              int port_index = -1;
              uint32_t num_ports = lilv_plugin_get_num_ports(plugin);
              for (uint32_t i = 0; i < num_ports && port_index == -1; ++i) {
                 const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
                 if (lilv_port_is_a(plugin, port, atom_AtomPort) /*&&
                     lilv_port_has_property(plugin, port, atom_supports)*/) {

                    // Get values of atom:supports
                    const LilvNodes* values = lilv_port_get_value(plugin, port, atom_supports);
                    LILV_FOREACH(nodes, j, values) {
                        const LilvNode* val = lilv_nodes_get(values, j);
                        if (lilv_node_equals(val, patch_Message)) {
                           port_index = i;
                        }
                    }
                 }
              }

              lilv_node_free(atom_AtomPort);
              lilv_node_free(atom_supports);
              lilv_node_free(patch_Message);

              if (port_index == -1) {
                pw_log_error("Found no port that supports atoms and patch messages");
              } else {
                pw_log_debug("Patch parameter using port %d", port_index);
                LV2_Atom_Forge forge;
                uint8_t buffer[1000];
                LV2_Atom_Forge_Frame frame;

                LV2_URID parameter_key = constants_map(constants, key);
                lv2_atom_forge_init(&forge, &constants.map);

                lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));
                lv2_atom_forge_object(&forge, &frame, 0, constants.patch_Set);
                lv2_atom_forge_key(&forge, constants.patch_property);
                lv2_atom_forge_urid(&forge, parameter_key);
                lv2_atom_forge_key(&forge, constants.patch_value);
                lv2_atom_forge_string(&forge, value, strlen(value));
                lv2_atom_forge_pop(&forge, &frame);

                ports_write(NULL, port_index, ((LV2_Atom*)buffer)->size + sizeof(LV2_Atom), constants.atom_eventTransfer, buffer);
              }

          }
      }
    }
    return 0;
}

