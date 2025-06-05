/*
 * ============================================================================
 *  File:       constants.h
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      lv2 uri/urid mapping/unmapping functions.
 *      
 * ============================================================================
 */

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <lv2/parameters/parameters.h>
#include <lv2/worker/worker.h>
#include <pipewire/array.h>

/* ========================================================================== */
/*                               Public Macros                                */
/* ========================================================================== */
#define constants_map(c, uri) ((c).map.map((c).map.handle, (uri)))
#define constants_unmap(c, uid) ((c).unmap.unmap((c).unmap.handle, (uid)))

/* ========================================================================== */
/*                              Public Typedefs                               */
/* ========================================================================== */
typedef struct URITable {
   struct pw_array array;
} URITable;

struct constants {
   LilvWorld *world;

   LilvNode *lv2_InputPort;
   LilvNode *lv2_OutputPort;
   LilvNode *lv2_AudioPort;
   LilvNode *lv2_ControlPort;
   LilvNode *lv2_Optional;
   LilvNode *atom_AtomPort;
   LilvNode *urid_map;
   LilvNode *powerOf2BlockLength;
   LilvNode *fixedBlockLength;
   LilvNode *boundedBlockLength;
   LilvNode *worker_schedule;
   LilvNode *worker_iface;
   LilvNode *rdfs_label;
   LilvNode *pset_Preset;

   URITable uri_table;
   LV2_URID_Map map;
   LV2_Feature map_feature;
   LV2_URID_Unmap unmap;
   LV2_Feature unmap_feature;

   LV2_URID atom_Int;
   LV2_URID atom_Float;
   LV2_URID atom_String;
   LV2_URID atom_Object;
   LV2_URID atom_Vector;
   LV2_URID atom_Urid;
   LV2_URID patch_Set;
   LV2_URID patch_Get;
   LV2_Atom_Forge forge;
   LV2_URID midi_MidiEvent;
   LV2_URID atom_Chunk;
   LV2_URID atom_Sequence;
   LV2_URID atom_eventTransfer;
};

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */
extern struct constants constants;

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
extern void constants_init();
