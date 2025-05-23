#ifndef ENGINEGROUP_H
#define ENGINEGROUP_H

#include <stdbool.h>

typedef struct {
   char *name;
   char *plugin;
   char *preset;
   bool showui;
   bool worker;
   bool engineworker;
} EngineSpec;

typedef struct {
   char *group;
   EngineSpec *engines;
   int engine_count;
} EngineGroup;

extern EngineGroup *enginegroup_parse(char *jsonstring);
extern void enginegroup_free(EngineGroup *group);

#endif
