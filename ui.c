#include <gtk/gtk.h>
#include <lilv/lilv.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/ui/ui.h>
#include <pipewire/filter.h>
#include <suil/suil.h>

#include "constants.h"
#include "engine_data.h"
#include "ports.h"

uint32_t ui_port_index(void* const controller, const char* symbol) {
   printf("\nui_port_index(%s)", symbol);
   fflush(stdout);
   Engine* engine = (Engine*)controller;

   for (int n = 0; n < engine->n_ports; n++) {
      if (!strcmp(symbol, engine->ports[n].name)) return engine->ports[n].index;
   }

   return LV2UI_INVALID_PORT_INDEX;
}

int pluginui_on_start(struct spa_loop* loop, bool async, uint32_t seq, const void* data,
                      size_t size, void* user_data) {
   Engine* engine = (Engine*)user_data;
   const LilvNode* selected_ui_type;

   printf("\nSUIL section start");
   fflush(stdout);

   SuilHost* suil_host = suil_host_new(ports_write_port, ui_port_index, NULL, NULL);

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
