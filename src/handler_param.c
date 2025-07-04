/*
 * ============================================================================
 *  File:       handler_param.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for pipewire params.
 *
 * ============================================================================
 */

#include <spa/pod/parser.h>
#include <spa/param/props.h>
#include "handler.h"
#include "node.h"

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
void on_param_changed(void *data, void *port_data, uint32_t id, const struct spa_pod *param) {
   Node *n = (Node *) data;
    if (id == SPA_PARAM_Props && param) {
        spa_pod_parse_object(param,
            SPA_TYPE_OBJECT_Props, NULL,
            SPA_PROP_volume, SPA_POD_OPT_Float(&n->gain));
    }
      struct spa_dict_item items[1];
      char sgain[20];
      sprintf(sgain,"%f",n->gain);
      items[0] = SPA_DICT_ITEM_INIT("elvira.gain", sgain);
      pw_filter_update_properties(node->filter, NULL, &SPA_DICT_INIT(items, 1));
}
