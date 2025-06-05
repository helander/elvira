#include "handler.h"
#include "node.h"

void on_destroy(void *data) {
   if (node->filter) pw_filter_destroy(node->filter);
}
