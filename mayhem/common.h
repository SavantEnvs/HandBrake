#ifndef MAYHEM_COMMON_H
#define MAYHEM_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "handbrake/handbrake.h"

static int mayhem_inited;

static void mayhem_init(void)
{
    if (!mayhem_inited)
    {
        hb_global_init_no_hardware();
        mayhem_inited = 1;
    }
}

/* NUL-terminated heap copy of the fuzz input. */
static char *mayhem_cstr(const uint8_t *data, size_t size)
{
    char *s = malloc(size + 1);
    if (s == NULL)
        abort();
    memcpy(s, data, size);
    s[size] = '\0';
    return s;
}

#endif
