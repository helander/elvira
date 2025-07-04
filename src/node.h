/*
 * ============================================================================
 *  File:       node.h
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Pipewire node functions.
 *      
 * ============================================================================
 */

#pragma once

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include "set.h"

/* ========================================================================== */
/*                               Public Macros                                */
/* ========================================================================== */

/* ========================================================================== */
/*                              Public Typedefs                               */
/* ========================================================================== */
typedef struct Node Node;
typedef struct NodePort NodePort;

struct Node {
   uint32_t node_id;
   struct pw_registry *registry;
   struct pw_filter *filter;
   int64_t clock_time;
   Set ports;
   float gain; 
   float previous_gain;
};

typedef enum {
   NODE_NONE,
   NODE_CONTROL_INPUT,
   NODE_CONTROL_OUTPUT,
   NODE_AUDIO_INPUT,
   NODE_AUDIO_OUTPUT
} NodePortType;

struct NodePort {
   int index;
   NodePortType type;
   struct pw_filter_port *pwPort;
   struct pw_buffer *pwbuffer;
};

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
extern Node *node;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
extern int node_setup();
