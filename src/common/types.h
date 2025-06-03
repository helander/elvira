#pragma once

/*
#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>
#include <lv2/worker/worker.h>
#include <pipewire/filter.h>
#include <pipewire/pipewire.h>
#include <spa/utils/ringbuffer.h>
#include <stdint.h>
#include <suil/suil.h>
*/

#include <stdbool.h>


#define ATOM_BUFFER_SIZE 16 * 1024

#define ATOM_RINGBUFFER_SIZE 1024 * 16 /* should be power of 2 */
#define MAX_ATOM_MESSAGE_SIZE 256

#define WORK_RESPONSE_RINGBUFFER_SIZE 1024 /* should be power of 2 */
#define MAX_WORK_RESPONSE_MESSAGE_SIZE 128

// Enable forward references
typedef struct Engine Engine;
typedef struct Node Node;
typedef struct Host Host;
typedef struct NodePort NodePort;
typedef struct HostPort HostPort;
typedef struct EnginePort EnginePort;

//

struct Engine {
   bool started;
   char setname[50];
   char enginename[50];
   char plugin_uri[200];
   char preset_uri[200];
   int samplerate;
   Node *node;
   Host *host;
   EnginePort *ports;
};
