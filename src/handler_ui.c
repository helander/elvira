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
#include <lilv/lilv.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/ui/ui.h>
#include <stdio.h>
#include <suil/suil.h>

#include "constants.h"
#include "handler.h"
#include "host.h"
#include "node.h"
#include "ports.h"
#include "runtime.h"

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
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

   const char* host_type_uri = "http://lv2plug.in/ns/extensions/ui#Gtk3UI";
   LilvNode* host_type = lilv_new_uri(constants.world, host_type_uri);
   if (!lilv_ui_is_supported(selectedUI, suil_ui_supported, host_type, &selected_ui_type)) {
      selectedUI = NULL;
   }
   lilv_node_free(host_type);

   pw_log_info("Plugin: %s", lilv_node_as_string(lilv_plugin_get_uri(host->lilvPlugin)));
   pw_log_info("Selected UI: %s", lilv_node_as_string(lilv_ui_get_uri(selectedUI)));
   pw_log_info("Selected UI type: %s", lilv_node_as_string(selected_ui_type));

   const LV2_Feature instance_feature = {LV2_INSTANCE_ACCESS_URI,
                                         lilv_instance_get_handle(host->instance)};

   const LV2_Feature idle_feature = {LV2_UI__idleInterface, NULL};

   const LV2_Feature* ui_features[] = {&constants.map_feature, &constants.unmap_feature,
                                       &instance_feature, &idle_feature, NULL};

   GtkWidget* plugin_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(plugin_window), config_nodename);
   gtk_window_set_default_size(GTK_WINDOW(plugin_window), 200, 150);
   gtk_widget_show_all(plugin_window);

   host->suil_instance = suil_instance_new(
       suil_host, (void*)host, "http://lv2plug.in/ns/extensions/ui#Gtk3UI",
       lilv_node_as_string(lilv_plugin_get_uri(host->lilvPlugin)),
       lilv_node_as_string(lilv_ui_get_uri(selectedUI)), lilv_node_as_string(selected_ui_type),
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
   return 0;
}

// seq is used to pass the port index and data passes the atom
int on_port_event_aseq(struct spa_loop* loop, bool async, uint32_t port_index, const void* data,
                       size_t size, void* user_data) {
   LV2_Atom_Sequence* aseq = (LV2_Atom_Sequence*)data;
   if (host->suil_instance) {
      LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
      if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
         long payloadSize = aseq->atom.size;
         while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
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
   if (host->suil_instance)
      suil_instance_port_event(host->suil_instance, port_index, size, constants.atom_eventTransfer,
                               atom);
}
