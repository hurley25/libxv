/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_atomic_test.c 08/12/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "xv_test.h"
#include "xv_atomic.h"

void *add_op(void *args)
{
    xv_atomic_t *pvalue = (xv_atomic_t *)args;
    for (int i = 0; i < 50000; ++i) {
        xv_atomic_add(pvalue, 1);
    }
    return NULL;
}

void *sub_op(void *args)
{
    xv_atomic_t *pvalue = (xv_atomic_t *)args;
    for (int i = 0; i < 50000; ++i) {
        xv_atomic_sub(pvalue, 1);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    xv_atomic_t value;

    xv_atomic_set(&value, 0);
    CHECK(xv_atomic_get(&value) == 0, "xv_automic_get(value) != 0");

    pthread_t id[3];
    int ret = pthread_create(&id[0], NULL, add_op, &value);
    ASSERT(ret == 0);
    ret = pthread_create(&id[1], NULL, sub_op, &value);
    ASSERT(ret == 0);
    ret = pthread_create(&id[2], NULL, add_op, &value);
    ASSERT(ret == 0);

    for (int i = 0; i < 3; ++i) {
        ret = pthread_join(id[i], NULL);
        ASSERT(ret == 0);
    }

    fprintf(stderr, "count = %d\n", xv_atomic_get(&value));
    CHECK(xv_atomic_get(&value) == 50000, "xv_automic_get(value) != 50000");

    return EXIT_SUCCESS;
}

