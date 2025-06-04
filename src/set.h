#pragma once

#include <stdlib.h>

typedef struct {
    void **items;
    size_t count;
    size_t capacity;
} Set;

void set_init(Set *set);

void set_free(Set *set);

void set_add(Set *set, void *item);

#define SET_FOR_EACH(type, var, set_ptr) \
    for (size_t _i = 0; _i < (set_ptr)->count && ((var) = (type)(set_ptr)->items[_i], 1); ++_i)
