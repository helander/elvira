#include "set.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_init(Set *set) {
    set->count = 0;
    set->capacity = 4;
    set->items = malloc(set->capacity * sizeof(void *));
}

void set_free(Set *set) {
    free(set->items);
}

void set_add(Set *set, void *item) {
    if (set->count == set->capacity) {
        set->capacity *= 2;
        void **new_items = realloc(set->items, set->capacity * sizeof(void *));
        set->items = new_items;
    }
    set->items[set->count++] = item;
}

