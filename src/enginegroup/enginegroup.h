#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
   char *name;
   char *plugin;
   char *preset;
   bool showui;
   uint32_t samplerate;
   uint32_t latency;
} EngineSpec;

typedef struct {
   char *group;
   EngineSpec *engines;
   int engine_count;
} EngineGroup;

extern EngineGroup *enginegroup_parse(const char *jsonstring);
extern void enginegroup_free(EngineGroup *group);

