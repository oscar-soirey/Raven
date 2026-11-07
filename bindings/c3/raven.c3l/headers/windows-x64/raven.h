#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifndef __c3__
#define __c3__

typedef void* c3typeid_t;
typedef void* c3fault_t;
typedef struct { void* ptr; size_t len; } c3slice_t;
typedef struct { void* ptr; c3typeid_t type; } c3any_t;

#endif

/* TYPES */
