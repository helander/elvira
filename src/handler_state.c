/*
 * ============================================================================
 *  File:       handler_state.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for state changes of the node.
 *      
 * ============================================================================
 */

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
void on_state_changed(void *userdata, enum pw_filter_state old, enum pw_filter_state state, const char *error)
{
    Node *node = (Node *) userdata;
    pw_log_debug("State change: Node Id %u State %d", pw_filter_get_node_id(node->filter), state);
    if (node->node_id == 0) {
        uint32_t id = pw_filter_get_node_id(node->filter);
        if (id != SPA_ID_INVALID) {
          node->node_id = id;
          pw_log_info("SetNode Id: %u", node->node_id);
        }
    }
}

