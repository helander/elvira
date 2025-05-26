#include "controller.h"

#include <pipewire/pipewire.h>

#include <spa/debug/types.h>
#include <spa/pod/iter.h>
#include <stdio.h>
#include <string.h>

#include "enginegroup.h"

#include "stb_ds.h"

#include "engine_data.h"
#include "engine.h"


typedef struct {
   struct pw_thread_loop *master_loop;
   struct pw_filter *filter;
   Engine* engines;
   char master_group[100];
} Controller;

static Controller controller;


static void run_engines() {
   for (int i = 0; i < arrlenu(controller.engines); i++) {
       Engine *engine = &controller.engines[i];
       engine->pw.master_loop = controller.master_loop;
       pw_loop_invoke(pw_thread_loop_get_loop(controller.master_loop), engine_entry, 0, NULL, 0, false, engine);
   }
}


static void on_command(void *data, const struct spa_command *command) {
   Engine *engine = (Engine *)data;
   if (SPA_NODE_COMMAND_ID(command) == SPA_NODE_COMMAND_User) {
      if (SPA_POD_TYPE(&command->pod) == SPA_TYPE_Object) {
         const struct spa_pod_object *obj = (const struct spa_pod_object *) &command->pod;
         struct spa_pod_prop *prop;
         SPA_POD_OBJECT_FOREACH(obj, prop) {
             if (prop->key == SPA_COMMAND_NODE_extra) {
                const struct spa_pod *value = &prop->value;
                if (SPA_POD_TYPE(value) == SPA_TYPE_String) {
                   const char *command_string = SPA_POD_BODY(value);

      printf("\nCommand---[%s]", command_string);fflush(stdout);

      const char  *json;
      if (strncmp(command_string,"add ", 4) == 0) {
         json = command_string + 4;
         printf("\nAdd command [%s]", json);
         EngineGroup *engines = enginegroup_parse(json);
         if (engines) {
             controller_add(engines);
             enginegroup_free(engines);
             run_engines();
         }
      } else {
         printf("\nUnknown command [%s]", command_string);
      }
      fflush(stdout);

                }
             }
         }
      }
   }
} 




static const struct pw_filter_events controller_events = {
    PW_VERSION_FILTER_EVENTS,
    //    .process = on_process,
        .command = on_command,
    //    .destroy = on_filter_destroy,
    //    .param_changed = on_param_changed,
};

void controller_init() {
  controller.engines = NULL;
  controller.master_group[0] = 0;
}

void controller_add(EngineGroup *enginegroup) {
   printf("\nGroup: \"%s\"", enginegroup->group);
   for (int i = 0; i < enginegroup->engine_count; ++i) {
      printf("\n  Engine: %s", enginegroup->engines[i].name);
      printf("\n    Plugin: %s", enginegroup->engines[i].plugin);
      printf("\n    Preset: %s", enginegroup->engines[i].preset);
      printf("\n    Showui: %s", enginegroup->engines[i].showui ? "true" : "false");
       Engine *engine  = (Engine *) calloc(1, sizeof(Engine));
       engine->started = false;
       engine_defaults(engine);
       engine->host.start_ui = enginegroup->engines[i].showui;
       strcpy(engine->groupname,enginegroup->group);
       strcpy(engine->enginename,enginegroup->engines[i].name);
       strcpy(engine->plugin_uri,enginegroup->engines[i].plugin);
       if (enginegroup->engines[i].samplerate) engine->pw.samplerate = enginegroup->engines[i].samplerate;
       if (enginegroup->engines[i].latency) engine->pw.latency_period = enginegroup->engines[i].latency;
      arrput(controller.engines, *engine);
   }
   fflush(stdout);
   if (!strlen(controller.master_group)) 
      strncpy(controller.master_group,enginegroup->group,sizeof(controller.master_group)-1); 
}


void controller_start() {
   controller.master_loop = pw_thread_loop_new("master", NULL);

   controller.filter = pw_filter_new_simple(pw_thread_loop_get_loop(controller.master_loop), controller.master_group,
                            pw_properties_new(PW_KEY_MEDIA_TYPE, "elvira", PW_KEY_MEDIA_ROLE, "controller", PW_KEY_MEDIA_NAME, "controller", NULL),
                            &controller_events, &controller);


   if (pw_filter_connect(controller.filter, PW_FILTER_FLAG_DRIVER, NULL, 0) < 0) {
      fprintf(stderr, "can't connect\n");
      return;
   }

       pw_filter_add_port(controller.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
                          pw_properties_new(PW_KEY_FORMAT_DSP, "control:f32", PW_KEY_PORT_NAME,
                                            "fakeport", NULL),
                          NULL, 0);



   pw_thread_loop_start(controller.master_loop);

   run_engines();
   printf("\nController started");
   fflush(stdout);
}
 

