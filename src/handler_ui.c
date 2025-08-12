/*
 * ============================================================================
 *  File:       handler_ui.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for plugin ui related functions.
 *      
 * ============================================================================
 */


#include <gtk/gtk.h>
#include <suil/suil.h>
#include <lilv/lilv.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/ui/ui.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "constants.h"
#include "handler.h"
#include "host.h"
#include "node.h"
#include "ports.h"
#include "runtime.h"
#include "util.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */

static int find_ui_binary(const char* bundle_dir,
                          const char* ui_uri_str,
                          char* binary_out, size_t binary_size)
{
    LilvWorld* world = lilv_world_new();
    LilvNode* bundle_uri = lilv_new_file_uri(world, NULL, bundle_dir);
    lilv_world_load_bundle(world, bundle_uri);

    LilvNode* ui_uri   = lilv_new_uri(world, ui_uri_str);
    LilvNode* p_binary = lilv_new_uri(world, LV2_CORE__binary);

    LilvNodes* binaries = lilv_world_find_nodes(world, ui_uri, p_binary, NULL);
    if (binaries && lilv_nodes_size(binaries) > 0) {
        const char* binary_path = lilv_uri_to_path(
            lilv_node_as_uri(lilv_nodes_get_first(binaries)));

        const char* slash = strrchr(binary_path, '/');
        snprintf(binary_out, binary_size,
                 "%s", slash ? slash + 1 : binary_path);

        lilv_nodes_free(binaries);
        lilv_node_free(ui_uri);
        lilv_node_free(p_binary);
        lilv_node_free(bundle_uri);
        lilv_world_free(world);
        return 0;
    }

    // Not found
    lilv_nodes_free(binaries);
    lilv_node_free(ui_uri);
    lilv_node_free(p_binary);
    lilv_node_free(bundle_uri);
    lilv_world_free(world);
    return -1;
}



static uint32_t ui_port_index(void* const controller, const char* symbol) {
   HostPort* port;
   SET_FOR_EACH(HostPort*, port, &host->ports) {
      if (!strcmp(symbol, port->name)) return port->index;
   }
   return LV2UI_INVALID_PORT_INDEX;
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
int on_ui_start(struct spa_loop* loop, bool async, uint32_t seq, const void* data, size_t size,
                void* user_data) {
   const LilvNode* selected_ui_type;

   SuilHost* suil_host = suil_host_new(ports_write, ui_port_index, NULL, NULL);

   const LilvInstance* const instance = host->instance;

   // Get UIs for the plugin
   const LilvUIs* uis = lilv_plugin_get_uis(host->lilvPlugin);
   LilvUI* selectedUI = NULL;
   LILV_FOREACH(uis, j, uis) {
      const LilvUI* ui = lilv_uis_get(uis, j);
      selectedUI = (LilvUI*)ui;
      const LilvNode* ui_uri = lilv_ui_get_uri(ui);
      const LilvNode* binary_uri = lilv_ui_get_binary_uri(ui);
      const LilvNode* bundle_uri = lilv_ui_get_bundle_uri(ui);
   }

   if (selectedUI) {
     const char* host_type_uri = "http://lv2plug.in/ns/extensions/ui#Gtk3UI";
//     const char* host_type_uri = "http://lv2plug.in/ns/extensions/ui#UI";
     LilvNode* host_type = lilv_new_uri(constants.world, host_type_uri);
     if (!lilv_ui_is_supported(selectedUI, suil_ui_supported, host_type, &selected_ui_type)) {
        selectedUI = NULL;
     }
     lilv_node_free(host_type);
     if (!selectedUI) {
        pw_log_error("UI not supported by this host.");
     }
   } else {
     pw_log_warn("No UI available for this plugin.");
   }

   pw_log_info("Plugin: %s", lilv_node_as_string(lilv_plugin_get_uri(host->lilvPlugin)));
   if (selectedUI) {
     pw_log_info("Selected UI: %s", lilv_node_as_string(lilv_ui_get_uri(selectedUI)));
     pw_log_info("Selected UI type: %s", lilv_node_as_string(selected_ui_type));
   }
   const LV2_Feature instance_feature = {LV2_INSTANCE_ACCESS_URI,
                                         lilv_instance_get_handle(host->instance)};

   const LV2_Feature idle_feature = {LV2_UI__idleInterface, NULL};

   const LV2_Feature* ui_features[] = {&constants.map_feature, &constants.unmap_feature,
                                       &instance_feature, &idle_feature, NULL};

   char title[100];
   strcpy(title, config_nodename);
   if (!selectedUI) strcat(title, " -- no UI available");
   GtkWidget* plugin_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(plugin_window), title);
   gtk_window_set_default_size(GTK_WINDOW(plugin_window), 400, 150);
   gtk_widget_show_all(plugin_window);

   if (selectedUI) {
     host->suil_instance = suil_instance_new(
       suil_host,
       (void*)host,
       "http://lv2plug.in/ns/extensions/ui#Gtk3UI",
       lilv_node_as_string(lilv_plugin_get_uri(host->lilvPlugin)),
       lilv_node_as_string(lilv_ui_get_uri(selectedUI)),
       lilv_node_as_string(selected_ui_type),
       lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_bundle_uri(selectedUI)), NULL),
       lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_binary_uri(selectedUI)), NULL),
       ui_features);

     if (host->suil_instance) {
      GtkWidget* plugin_widget = suil_instance_get_widget(host->suil_instance);
      gtk_container_add(GTK_CONTAINER(plugin_window), plugin_widget);
      gtk_widget_show_all(plugin_window);
     } else {
      pw_log_error("Could not create UI for %s", config_nodename);
     }
   }

   const char* ui_uri       = "http://helander.network/lv2ui/madigan";
   const char* ui_type_uri  = "http://lv2plug.in/ns/extensions/ui#UI"; 
   const char* bundle_dir   = "/usr/lib/lv2/madigan.lv2";

   char binary_file[256];
   if (find_ui_binary(bundle_dir, ui_uri, binary_file, sizeof(binary_file)) != 0) {
        fprintf(stderr, "Error: Could not find lv2:binary for UI %s in %s\n",
                ui_uri, bundle_dir);
   } else {
       char binary_path[512];
       snprintf(binary_path, sizeof(binary_path), "%s/%s", bundle_dir, binary_file);
       SuilInstance* inst = suil_instance_new(
          suil_host,
          (void*)host,
          NULL,
          lilv_node_as_string(lilv_plugin_get_uri(host->lilvPlugin)),
          ui_type_uri,
          bundle_dir,
          bundle_path,
          ui_features
       );
       if (!inst) {
           pw_log_error("Failed to create suil instance for Madigan UI");
       } else {
          pw_log_info("Madigan UI instance created successfully for plugin");
       }
   }

   return 0;
}

// seq is used to pass the port index and data passes the atom
int on_port_event_aseq(struct spa_loop* loop, bool async, uint32_t port_index, const void* data,
                       size_t size, void* user_data) {
   LV2_Atom_Sequence* aseq = (LV2_Atom_Sequence*)data;
   pw_log_debug("PORT EVENT aseq");
   if (host->suil_instance) {
      LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
      if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
         long payloadSize = aseq->atom.size;
         while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
            //util_print_atom(&aev->body);
            suil_instance_port_event(host->suil_instance, port_index, aev->body.size,
                                     constants.atom_eventTransfer, &aev->body);
            int eventSize =
                lv2_atom_pad_size(sizeof(LV2_Atom_Event)) + lv2_atom_pad_size(aev->body.size);
            char* next = ((char*)aev) + eventSize;
            payloadSize = payloadSize - eventSize;
            aev = (LV2_Atom_Event*)next;
         }
      }
   }
}

// seq is used to pass the port index and data passes the atom
int on_port_event_atom(struct spa_loop* loop, bool async, uint32_t port_index, const void* atom,
                       size_t size, void* user_data) {
   pw_log_debug("PORT EVENT atom");
   //util_print_atom(atom);
   if (host->suil_instance)
      suil_instance_port_event(host->suil_instance, port_index, size, constants.atom_eventTransfer,
                               atom);
}



// seq is used to pass the port index and data passes the midi data
int on_output_midi_event(struct spa_loop* loop, bool async, uint32_t port_index, const void* data,
                       size_t size, void* user_data) {
   uint8_t *mididata = (uint8_t *)data;
   if (mididata[0] == 0xB0) {
     // midi CC channel 0
     int cc = mididata[1];
     int val = mididata[2];
     pw_log_debug("Output Midi CC %d %d",mididata[0],cc,val);

     struct spa_dict_item items[1];
     char prop_name[100];
     sprintf(prop_name,"elvira.midicc.%d",cc);
     char prop_value[20];
     sprintf(prop_value,"%d",val);
     items[0] = SPA_DICT_ITEM_INIT(prop_name, prop_value);
     pw_filter_update_properties(node->filter, NULL, &SPA_DICT_INIT(items, 1));
   } else {
     pw_log_debug("Output Midi 0x%02x 0x%02x 0x%02x",mididata[0],mididata[1],mididata[2]);
   }
}


// seq is used to pass the port index and data passes the midi data
int on_input_midi_event(struct spa_loop* loop, bool async, uint32_t port_index, const void* data,
                       size_t size, void* user_data) {
   uint8_t *mididata = (uint8_t *)data;
   if (mididata[0] == 0xB0) {
     // midi CC channel 0
     int cc = mididata[1];
     int val = mididata[2];
     pw_log_debug("Input Midi CC 0x%02x %d %d",mididata[0],cc,val);

     struct spa_dict_item items[1];
     char prop_name[100];
     sprintf(prop_name,"elvira.midicc.%d",cc);
     char prop_value[20];
     sprintf(prop_value,"%d",val);
     items[0] = SPA_DICT_ITEM_INIT(prop_name, prop_value);
     pw_filter_update_properties(node->filter, NULL, &SPA_DICT_INIT(items, 1));
   } else {
     pw_log_debug("Input Midi 0x%02x 0x%02x 0x%02x",mididata[0],mididata[1],mididata[2]);
   }
}


