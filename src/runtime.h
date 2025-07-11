/*
 * ============================================================================
 *  File:       runtime.h
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Basic runtime objects.
 *      
 * ============================================================================
 */

#include <pipewire/pipewire.h>
#include <stdbool.h>

/* ========================================================================== */
/*                               Public Macros                                */
/* ========================================================================== */

/* ========================================================================== */
/*                              Public Typedefs                               */
/* ========================================================================== */

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
extern struct pw_thread_loop *runtime_primary_event_loop;
extern struct pw_thread_loop *runtime_worker_event_loop;

extern int config_samplerate;
extern int config_latency_period;
extern bool config_show_ui;
extern char *config_plugin_uri;
extern char *config_preset_uri;
extern char *config_nodename;
extern char *config_group;
extern char *config_step;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
extern void runtime_init();

