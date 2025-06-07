/*
 * ============================================================================
 *  File:       constants.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      lv2 uri/urid mapping/unmapping functions.
 *      
 * ============================================================================
 */

#include "constants.h"

#include <lilv/lilv.h>
#include <lv2/patch/patch.h>
#include <lv2/presets/presets.h>
#include <pipewire/pipewire.h>

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */
static void uri_table_init(URITable *table) { pw_array_init(&table->array, 2048); }

static void uri_table_destroy(URITable *table) {
   char **p;
   pw_array_for_each(p, &table->array) free(*p);
   pw_array_clear(&table->array);
}

static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri) {
   size_t i = 0;

   URITable *table = (URITable *)handle;
   char **p;

   pw_array_for_each(p, &table->array) {
      i++;
      if (spa_streq(*p, uri)) goto done;
   }
   pw_array_add_ptr(&table->array, strdup(uri));
   i = pw_array_get_len(&table->array, char *);
done:

   pw_log_trace("uri_table_map  %s %d",uri,i);

   return i;
}

static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid) {
   URITable *table = (URITable *)handle;

   if (urid > 0 && urid <= pw_array_get_len(&table->array, char *))
      return *pw_array_get_unchecked(&table->array, urid - 1, char *);

   return NULL;
}

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
struct constants constants;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
void constants_init() {
   uri_table_init(&constants.uri_table);
   constants.world = lilv_world_new();
   lilv_world_load_all(constants.world);

   constants.lv2_InputPort = lilv_new_uri(constants.world, LV2_CORE__InputPort);
   constants.lv2_OutputPort = lilv_new_uri(constants.world, LV2_CORE__OutputPort);
   constants.lv2_AudioPort = lilv_new_uri(constants.world, LV2_CORE__AudioPort);
   constants.lv2_ControlPort = lilv_new_uri(constants.world, LV2_CORE__ControlPort);
   constants.lv2_Optional = lilv_new_uri(constants.world, LV2_CORE__connectionOptional);
   constants.atom_AtomPort = lilv_new_uri(constants.world, LV2_ATOM__AtomPort);
   constants.urid_map = lilv_new_uri(constants.world, LV2_URID__map);
   constants.powerOf2BlockLength = lilv_new_uri(constants.world, LV2_BUF_SIZE__powerOf2BlockLength);
   constants.fixedBlockLength = lilv_new_uri(constants.world, LV2_BUF_SIZE__fixedBlockLength);
   constants.boundedBlockLength = lilv_new_uri(constants.world, LV2_BUF_SIZE__boundedBlockLength);
   constants.worker_schedule = lilv_new_uri(constants.world, LV2_WORKER__schedule);
   constants.worker_iface = lilv_new_uri(constants.world, LV2_WORKER__interface);

   constants.map.handle = &constants.uri_table;
   constants.map.map = uri_table_map;
   constants.map_feature.URI = LV2_URID_MAP_URI;
   constants.map_feature.data = &constants.map;
   constants.unmap.handle = &constants.uri_table;
   constants.unmap.unmap = uri_table_unmap;
   constants.unmap_feature.URI = LV2_URID_UNMAP_URI;
   constants.unmap_feature.data = &constants.unmap;

   constants.atom_Int = constants_map(constants, LV2_ATOM__Int);
   constants.atom_Float = constants_map(constants, LV2_ATOM__Float);
   constants.atom_String = constants_map(constants, LV2_ATOM__String);
   constants.atom_Object = constants_map(constants, LV2_ATOM__Object);
   constants.atom_Vector = constants_map(constants, LV2_ATOM__Vector);
   constants.atom_Urid = constants_map(constants, LV2_ATOM__URID);
   constants.patch_Set = constants_map(constants, LV2_PATCH__Set);
   constants.patch_Get = constants_map(constants, LV2_PATCH__Get);
   lv2_atom_forge_init(&constants.forge, &constants.map);
   constants.midi_MidiEvent = constants_map(constants, LV2_MIDI__MidiEvent);
   constants.atom_Chunk = constants_map(constants, LV2_ATOM__Chunk);
   constants.atom_Sequence = constants_map(constants, LV2_ATOM__Sequence);
   constants.atom_eventTransfer = constants_map(constants, LV2_ATOM__eventTransfer);

   constants.rdfs_label = lilv_new_uri(constants.world, (LILV_NS_RDFS "label"));
   constants.pset_Preset = lilv_new_uri(constants.world, (LV2_PRESETS__Preset));
}
