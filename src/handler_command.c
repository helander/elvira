#include <spa/pod/iter.h>

#include "handler.h"
#include "runtime.h"

void on_command(void *data, const struct spa_command *command) {
   if (SPA_NODE_COMMAND_ID(command) == SPA_NODE_COMMAND_User) {
      if (SPA_POD_TYPE(&command->pod) == SPA_TYPE_Object) {
         const struct spa_pod_object *obj = (const struct spa_pod_object *)&command->pod;
         struct spa_pod_prop *prop;
         SPA_POD_OBJECT_FOREACH(obj, prop) {
            if (prop->key == SPA_COMMAND_NODE_extra) {
               const struct spa_pod *value = &prop->value;
               if (SPA_POD_TYPE(value) == SPA_TYPE_String) {
                  const char *command_string = SPA_POD_BODY(value);
                  char args[100];
                  if (sscanf(command_string, "preset %s", args) == 1) {
                     pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop),
                                    on_host_preset, 0, args, strlen(args) + 1, false, NULL);
                  } else if (sscanf(command_string, "save %s", args) == 1) {
                     pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop),
                                    on_host_save, 0, args, strlen(args) + 1, false, NULL);
                  } else {
                     printf("\nUnknown command [%s]", command_string);
                  }
               }
            }
         }
      }
   }
}
