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
   char *name;
   EngineSpec *engines;
   int engine_count;
} EngineSet;

extern EngineSet  *engineset_parse(const char *jsonstring);
extern void engineset_free(EngineSet *set);

