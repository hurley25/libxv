/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_atomic.h 08/12/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_ATOMIC_H_
#define XV_ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct xv_atomic_t {
    volatile int counter;
} xv_atomic_t;

static inline int xv_atomic_get(xv_atomic_t *atomic)
{
    return __atomic_load_n(&atomic->counter, __ATOMIC_SEQ_CST);
}

static inline void xv_atomic_set(xv_atomic_t *atomic, int value)
{
    return __atomic_store_n(&atomic->counter, value, __ATOMIC_SEQ_CST);
}

static inline int xv_atomic_add(xv_atomic_t *atomic, int value)
{
    return __sync_add_and_fetch(&atomic->counter, value);
}

static inline int xv_atomic_sub(xv_atomic_t *atomic, int value)
{
    return __sync_sub_and_fetch(&atomic->counter, value);
}

static inline int xv_atomic_incr(xv_atomic_t *atomic)
{
    return xv_atomic_add(atomic, 1);
}

static inline int xv_atomic_decr(xv_atomic_t *atomic)
{
    return xv_atomic_sub(atomic, 1);
}

#ifdef __cplusplus
}
#endif

#endif // XV_ATOMIC_H_

