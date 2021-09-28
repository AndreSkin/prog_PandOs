#ifndef LIBRARIES_H_INCLUDED
#define LIBRARIES_H_INCLUDED

#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "umps3/umps/libumps.h"

/* Sostituisce memcpy */
void memcpy(void *dest, const void *src, size_t n);

#endif
