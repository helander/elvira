
/*
 * ============================================================================
 *  File:       handler_metadata.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for metadata changes.
 *      
 * ============================================================================
 */

#include "handler.h"
#include "runtime.h"
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
int on_metadata_property(void *object,
                                 uint32_t subject,
                                 const char *key,
                                 const char *type,
                                 const char *value)
{
    Node *node = (Node *) object;
    pw_log_debug("Metadata update: node=%u  key=%s  value=%s  subject %u", node->node_id, key, value, subject);
    if (subject == node->node_id) {
      pw_log_debug("Subject match:  key=%s  value=%s", key, value);
      if (!strcmp(key,"use_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_preset, 0, value, strlen(value) + 1, false, NULL);
      } else if (!strcmp(key,"save_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_save, 0, value, strlen(value) + 1, false, NULL);
      }
    }
   return 0;
}
