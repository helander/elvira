#include "set.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_init(Set *set) {
printf("\nsetinit %p",set);fflush(stdout);
    set->count = 0;
printf("\nsetinit 2");fflush(stdout);
    set->capacity = 4;
printf("\nsetinit 3");fflush(stdout);
    set->items = malloc(set->capacity * sizeof(void *));
printf("\nsetinit 4");fflush(stdout);
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

