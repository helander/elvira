#pragma once

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include "set.h"

typedef struct Node Node;
typedef struct NodePort NodePort;

struct Node {
   struct pw_filter *filter;
   int64_t clock_time;
   Set ports;
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

extern struct pw_filter_events *node_get_engine_filter_events();
extern int node_setup();

extern Node *node;
