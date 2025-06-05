/*
 * ============================================================================
 *  File:       set.c
 *  Project:    elvira
 *  Author:     Lars-Erik Helander <lehswel@gmail.com>
 *  License:    MIT
 *
 *  Description:
 *      Simple set collection support..
 *      
 * ============================================================================
 */

#include "set.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/*                               Local State                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                              Local Functions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                               Public State                                 */
/* ========================================================================== */

/* ========================================================================== */
/*                             Public Functions                               */
/* ========================================================================== */
void set_init(Set *set) {
   set->count = 0;
   set->capacity = 4;
   set->items = malloc(set->capacity * sizeof(void *));
}

void set_free(Set *set) { free(set->items); }

void set_add(Set *set, void *item) {
   if (set->count == set->capacity) {
      set->capacity *= 2;
      void **new_items = realloc(set->items, set->capacity * sizeof(void *));
      set->items = new_items;
   }
   set->items[set->count++] = item;
}
