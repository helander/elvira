#include <gtk/gtk.h>
#include <lilv/lilv.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/ui/ui.h>
#include <suil/suil.h>

#include "constants.h"
#include "stb_ds.h"
#include "types.h"
//#include "ports.h"


#include <stdio.h>

//#include "util.h"

static
EnginePort *find_engine_port(Engine *engine, uint32_t port_index) {
    for (int n = 0; n < arrlen(engine->ports); n++ ) {                                                                                                                                        
       EnginePort *port = &engine->ports[n];                                                                                                                                                    
       if (!port->host_port) continue;
       if (port->host_port->index == port_index) {                                                                                                                                                      
          return port;                                                                                                                                                                        
       }
    }
    return NULL;
}

//skall flyttas till engine_ports
void engine_ports_write(void *const controller, const uint32_t port_index, const uint32_t buffer_size,
                      const uint32_t protocol, const void *const buffer) {
   Engine *engine = (Engine *)controller;
   EnginePort *port = find_engine_port(engine, port_index);
   if (protocol == 0U) {
      const float value = *(const float *)buffer;
      printf("\nWrite to control port %d value %f", port_index, value);
      fflush(stdout);
      //  do something here ...
   } else if (protocol == constants.atom_eventTransfer) {
      const LV2_Atom *const atom = (const LV2_Atom *)buffer;
      if (buffer_size < sizeof(LV2_Atom) || (sizeof(LV2_Atom) + atom->size != buffer_size)) {
         printf("\nWrite to atom port %d canceled - wrong buffer size %d", port_index, buffer_size);
         fflush(stdout);
      } else {
         // printf("\n[%s]  Write to atom port %d - buffer size %d atom size %d  type %d %s",
         //        engine->enginename, port_index, buffer_size, atom->size, atom->type,
         //        constants_unmap(constants, atom->type));
         // fflush(stdout);

         uint16_t len = buffer_size;
         if (buffer_size > MAX_ATOM_MESSAGE_SIZE) {
            fprintf(stderr, "Payload too large\n");
         } else {
            uint8_t temp[MAX_ATOM_MESSAGE_SIZE + sizeof(uint16_t)];
            memcpy(temp, &len, sizeof(uint16_t));
            memcpy(temp + sizeof(uint16_t), buffer, len);
            uint32_t total_len = len + sizeof(uint16_t);

            uint32_t write_index;
            spa_ringbuffer_get_write_index(&port->ring, &write_index);

            uint32_t ring_offset = write_index & (ATOM_RINGBUFFER_SIZE - 1);
            uint32_t space = ATOM_RINGBUFFER_SIZE - ring_offset;

            if (space >= total_len) {
               memcpy(port->ringbuffer + ring_offset, temp, total_len);
            } else {
               // Wrap around
               memcpy(port->ringbuffer + ring_offset, temp, space);
               memcpy(port->ringbuffer, temp + space, total_len - space);
            }

            spa_ringbuffer_write_update(&port->ring, write_index + total_len);
         }
      }
   }
}




uint32_t ui_port_index(void* const controller, const char* symbol) {
   printf("\nui_port_index(%s)", symbol);
   fflush(stdout);
   Engine* engine = (Engine*)controller;

   for (int n = 0; n < arrlen(engine->host.ports); n++) {
      HostPort *port = &engine->host.ports[n];
      if (!strcmp(symbol, port->name)) return port->index;
   }

   return LV2UI_INVALID_PORT_INDEX;
}

int pluginui_on_start(struct spa_loop* loop, bool async, uint32_t seq, const void* data,
                      size_t size, void* user_data) {
   Engine* engine = (Engine*)user_data;
   const LilvNode* selected_ui_type;

   printf("\nSUIL section start");
   fflush(stdout);

   SuilHost* suil_host = suil_host_new(engine_ports_write, ui_port_index, NULL, NULL);

   const LilvInstance* const instance = engine->host.instance;

   // Get UIs for the plugin
   const LilvUIs* uis = lilv_plugin_get_uis(engine->host.lilvPlugin);
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

   printf("Plugin: %s\n\n", lilv_node_as_string(lilv_plugin_get_uri(engine->host.lilvPlugin)));
   printf("Selected UI: %s\n\n", lilv_node_as_string(lilv_ui_get_uri(selectedUI)));
   printf("Selected UI type: %s\n\n", lilv_node_as_string(selected_ui_type));

   const LV2_Feature instance_feature = {LV2_INSTANCE_ACCESS_URI,
                                         lilv_instance_get_handle(engine->host.instance)};

   const LV2_Feature idle_feature = {LV2_UI__idleInterface, NULL};

   const LV2_Feature* ui_features[] = {&constants.map_feature, &constants.unmap_feature,
                                       &instance_feature, &idle_feature, NULL};

   GtkWidget* plugin_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(plugin_window), engine->enginename);
   gtk_window_set_default_size(GTK_WINDOW(plugin_window), 200, 150);
   gtk_widget_show_all(plugin_window);

   engine->host.suil_instance = suil_instance_new(
       suil_host, (void*)engine, "http://lv2plug.in/ns/extensions/ui#Gtk3UI",
       lilv_node_as_string(lilv_plugin_get_uri(engine->host.lilvPlugin)),
       lilv_node_as_string(lilv_ui_get_uri(selectedUI)), lilv_node_as_string(selected_ui_type),
       lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_bundle_uri(selectedUI)), NULL),
       lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_binary_uri(selectedUI)), NULL),
       ui_features);

   if (engine->host.suil_instance) {
      GtkWidget* plugin_widget = suil_instance_get_widget(engine->host.suil_instance);
      gtk_container_add(GTK_CONTAINER(plugin_window), plugin_widget);
      gtk_widget_show_all(plugin_window);
   } else {
      printf("\nCould not create UI for %s", engine->enginename);
      fflush(stdout);
   }
   return 0;
}
