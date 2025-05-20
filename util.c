//#include "ports.h"

#include <lv2/atom/atom.h>
//#include <lv2/atom/util.h>
//#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
//#include <spa/control/control.h>
//#include <spa/control/ump-utils.h>
//#include <spa/pod/builder.h>
#include <stdio.h>

#include "constants.h"
//#include "node_data.h"


void util_print_atom(const LV2_Atom *atom) {
         if (atom->type != 0) {
            printf("\n   Atom type %d %s  size %d", atom->type,
                   constants_unmap(constants, atom->type), atom->size);

            if (atom->type == constants.atom_Int) {
               printf("\n        Int: %d", *(const int32_t *)(atom + 1));
            } else if (atom->type == constants.atom_Float) {
               printf("\n        Float: %f", *(const float *)(atom + 1));
            } else if (atom->type == constants.atom_Urid) {
               printf("\n        URID: %u (%s)", *(const uint32_t *)(atom + 1),
                      constants_unmap(constants, *(const uint32_t *)(atom + 1)));
            } else if (atom->type == constants.atom_String) {
               printf("\n        String: \"%s\"", (const char *)(atom + 1));
            } else if (atom->type == constants.atom_Object) {
               LV2_Atom_Object *obj = (LV2_Atom_Object *)atom;
               printf("\n        Object id %d  type %d (%s)", obj->body.id, obj->body.otype,
                      constants_unmap(constants, obj->body.otype));
               LV2_ATOM_OBJECT_FOREACH(obj, prop) {
                  printf("\n           Property: key URID=%u (%s), type=%u", prop->key,
                         constants_unmap(constants, prop->key), prop->value.type);
                  if (prop->value.type == constants.atom_Int) {
                     printf("      -> Int: %d", *(const int32_t *)(prop + 1));
                  } else if (prop->value.type == constants.atom_Float) {
                     printf("      -> Float: %f", *(const float *)(prop + 1));
                  } else if (prop->value.type == constants.atom_Urid) {
                     printf("      -> URID: %u (%s)", *(const uint32_t *)(prop + 1),
                            constants_unmap(constants, *(const uint32_t *)(prop + 1)));
                  } else if (prop->value.type == constants.atom_String) {
                     printf("      -> String: \"%s\"", (const char *)(prop + 1));
                  } else {
                     printf("      -> Unknown type");
                  }
               }
            } else if (atom->type == constants.atom_Vector) {
               LV2_Atom_Vector *vector = (LV2_Atom_Vector *)atom;
               printf("\n        Vector size %d  type %d (%s)", vector->body.child_size,
                      vector->body.child_type, constants_unmap(constants, vector->body.child_type));
            }
         }

}

void util_print_atom_sequence(const char *nodename, const char *direction, int port,
                         const LV2_Atom_Sequence *aseq) {
return;
   if (aseq->atom.type != constants.atom_Sequence) {
      printf("\n[%s]  Port %d %s  UNEXPECTED ?    Atom sequence of type %d %s)", nodename, port,
             direction, aseq->atom.type, constants_unmap(constants, aseq->atom.type));
      fflush(stdout);
   }
   LV2_Atom_Event *aev = (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq));
   if (aev->body.type == 0) return;
   if (aseq->atom.size > sizeof(LV2_Atom_Sequence)) {
      printf("\n[%s]  Port %d %s  Atom sequence (type %d %s size  %d)", nodename, port, direction,
             aseq->atom.type, constants_unmap(constants, aseq->atom.type), aseq->atom.size);
      long payloadSize = aseq->atom.size;
      while (payloadSize > (long)sizeof(LV2_Atom_Event)) {
         util_print_atom(&aev->body);
         int eventSize =
             lv2_atom_pad_size(sizeof(LV2_Atom_Event)) + lv2_atom_pad_size(aev->body.size);
         char *next = ((char *)aev) + eventSize;
         payloadSize = payloadSize - eventSize;
         aev = (LV2_Atom_Event *)next;
      }
   }
   fflush(stdout);
}


