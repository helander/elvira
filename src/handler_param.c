#include "handler.h"

void on_param_changed(void *data, void *port_data, uint32_t id, const struct spa_pod *param) {
   printf("\nParam changed type %d  size %d", param->type, param->size);
   if (param->type == SPA_TYPE_Object) printf("\nobject");
   fflush(stdout);
}
