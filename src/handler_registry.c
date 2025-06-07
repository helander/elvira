
/*
 * ============================================================================
 *  File:       handler_registry.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Event loop handler for registry events.
 *      
 * ============================================================================
 */

#include "handler.h"
#include "node.h"

#include <pipewire/extensions/metadata.h>
/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */
static struct spa_hook metadata_listener;

static const struct pw_metadata_events metadata_events = {
    PW_VERSION_METADATA_EVENTS,
    .property = on_metadata_property,
};

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
void on_registry_global(void *object,
                               uint32_t id,
                               uint32_t permissions,
                               const char *type,
                               uint32_t version,
                               const struct spa_dict *props)
{
    Node *node = (Node *) object;

    if (strcmp(type, "PipeWire:Interface:Metadata") == 0) {
        const char *name = spa_dict_lookup(props, "metadata.name");
        if (name && strcmp(name, "default") == 0) {
            struct pw_metadata *metadata = (struct pw_metadata *)
                pw_registry_bind(node->registry, id, type, version, 0);
            pw_metadata_add_listener(metadata,
                                     &metadata_listener,
                                     &metadata_events, node);
            pw_log_info("Connected to default metadata");
        }
    }
}

