
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
#include "ports.h"

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
      const char *control_prefix = "control.in.";
      if (!strcmp(key,"use_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_preset, 0, value, strlen(value) + 1, false, NULL);
      } else if (!strcmp(key,"save_preset")) {
         pw_loop_invoke(pw_thread_loop_get_loop(runtime_primary_event_loop), on_host_save, 0, value, strlen(value) + 1, false, NULL);
      } else if (!strncmp(key,control_prefix,strlen(control_prefix))) {
         float float_value = atof(value);
         const char *port_index_suffix = &key[strlen(control_prefix)];
         int port_index = atoi(port_index_suffix);
         ports_write(NULL, port_index, sizeof(float), 0U, &float_value);
      }
    }
    return 0;
}

/*
static const struct pw_metadata_events metadata_events = {
    PW_VERSION_METADATA_EVENTS,
    .property = on_metadata_property,
};

void setup_metadata_listener(struct pw_core *core)
{
    struct pw_metadata *metadata = pw_core_get_metadata(core, NULL, 0);
    static struct spa_hook metadata_listener;
    pw_metadata_add_listener(metadata, &metadata_listener, &metadata_events, NULL);
}
*/
