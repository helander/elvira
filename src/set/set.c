#include <string.h>
#include <stdlib.h>
#include "set.h"
#include "cJSON.h"

void engineset_free(EngineSet *set) {
   for (int i = 0; i < set->engine_count; ++i) {
      free(set->engines[i].name);
      free(set->engines[i].plugin);
      free(set->engines[i].preset);
   }
   free(set->engines);
}

EngineSet *engineset_parse(const char *jsonstring) {
   cJSON *cjson = cJSON_Parse(jsonstring);
   if (!cjson) return NULL;

   EngineSet *set = calloc(1,sizeof(EngineSet)); 

   cJSON *engineset = cJSON_GetObjectItem(cjson, "set");
   if (cJSON_IsString(engineset)) set->name = strdup(engineset->valuestring);

   set->engine_count = 0;
   cJSON *engines = cJSON_GetObjectItem(cjson, "engines");
   if (cJSON_IsArray(engines)) {
      int n_engines = cJSON_GetArraySize(engines);
      set->engines = calloc(n_engines, sizeof(EngineSpec));
      set->engine_count = n_engines;
      for (int i = 0; i < n_engines; ++i) {
         cJSON *engine = cJSON_GetArrayItem(engines, i);
         cJSON *name = cJSON_GetObjectItem(engine, "name");
         cJSON *plugin = cJSON_GetObjectItem(engine, "plugin");
         cJSON *preset = cJSON_GetObjectItem(engine, "preset");
         cJSON *showui = cJSON_GetObjectItem(engine, "showui");
         cJSON *samplerate = cJSON_GetObjectItem(engine, "samplerate");
         cJSON *latency = cJSON_GetObjectItem(engine, "latency");

         if (cJSON_IsString(name)) set->engines[i].name = strdup(name->valuestring);
         if (cJSON_IsString(plugin)) set->engines[i].plugin = strdup(plugin->valuestring);
         if (cJSON_IsString(preset)) set->engines[i].preset = strdup(preset->valuestring);
         if (cJSON_IsBool(showui)) set->engines[i].showui = cJSON_IsTrue(showui);
         if (cJSON_IsNumber(samplerate)) set->engines[i].samplerate = samplerate->valueint;
         if (cJSON_IsNumber(latency)) set->engines[i].latency = latency->valueint;
      }
   }

   cJSON_Delete(cjson);

   return set;
}
