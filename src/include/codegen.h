#ifndef CODEGEN_H
#define CODEGEN_H

#include "irgen.h"

#define SET_SIZE 100

struct interval;
typedef struct interval interval_t;
struct interval {
    unsigned short upper;
    unsigned short lower;
    char *sym;

    interval_t *next;
};

typedef struct interval_list {
    unsigned short count;
    interval_t *list;
} interval_list_t;

struct set_element;
typedef struct set_element set_element_t;
struct set_element {
    char *sym;
    set_element_t *next;
};

typedef struct set {
    set_element_t *s[SET_SIZE];
} set_t;

typedef struct register_pool {
    short pool[6];
    short index;
} register_pool_t;

void generate_code(quadblock_t*, env_t*);

#endif