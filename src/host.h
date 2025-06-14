/*
 * ============================================================================
 *  File:       host.h
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      lv2 host functions.
 *
 * ============================================================================
 */

#pragma once

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>
#include <lv2/worker/worker.h>
#include <spa/utils/ringbuffer.h>
#include <suil/suil.h>

#include "set.h"

/* ========================================================================== */
/*                               Public Macros                                */
/* ========================================================================== */
#define ATOM_BUFFER_SIZE 16 * 1024
#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128

/* ========================================================================== */
/*                              Public Typedefs                               */
/* ========================================================================== */
typedef struct Host Host;
typedef struct HostPort HostPort;

struct Host {
   const LilvPlugin *lilvPlugin;  // byt namn till lilv_plugin
   LilvNode *lilv_preset;
   LilvInstance *instance;  // byt namn till lilv_instance
   LV2_Handle handle;       // byt namn till lv2_handle
   const LV2_Worker_Interface *iface;
   void *worker_data;
   LV2_Worker_Schedule work_schedule;
   LV2_Feature work_schedule_feature;
   LV2_Options_Option options[6];
   LV2_Feature options_feature;
   const LV2_Feature *features[7];
   struct spa_ringbuffer work_response_ring;
   uint8_t work_response_buffer[WORK_RESPONSE_RINGBUFFER_SIZE];
   int32_t block_length;
   // bool start_ui;
   SuilInstance *suil_instance;

   // char plugin_uri[200];
   // char preset_uri[200];
   // HostPort *ports;
   Set ports;
};

typedef enum {
   HOST_NONE,
   HOST_CONTROL_INPUT,
   HOST_CONTROL_OUTPUT,
   HOST_AUDIO_INPUT,
   HOST_AUDIO_OUTPUT,
   HOST_ATOM_INPUT,
   HOST_ATOM_OUTPUT
} HostPortType;

struct HostPort {
   int index;
   HostPortType type;
   char name[100];
   const LilvPort *lilvPort;
   float dfault;               // not used by all
   float min;                  // not used by all
   float max;                  // not used by all
   LV2_Atom_Sequence *buffer;  // not used by all
   float current;              // not used by all
};

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
extern Host *host;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
extern int host_setup();
extern void host_ports_discover();
extern char *host_info_base();
extern char *host_info_ports();
