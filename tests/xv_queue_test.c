/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_queue_test.c 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "xv_test.h"
#include "xv_queue.h"

int main(int argc, char *argv[])
{
    xv_queue_t *queue = xv_queue_init();
    ASSERT(queue != NULL);
    ASSERT(xv_queue_size(queue) == 0);

    ASSERT(xv_queue_pop(queue) == NULL);
    ASSERT(xv_queue_pop(queue) == NULL);
    ASSERT(xv_queue_pop(queue) == NULL);

    ASSERT(xv_queue_size(queue) == 0);

    xv_queue_push(queue, (void *)1);

    ASSERT(xv_queue_size(queue) == 1);

    xv_queue_push(queue, (void *)2);
    xv_queue_push(queue, (void *)3);
    xv_queue_push(queue, (void *)4);
    xv_queue_push(queue, (void *)5);

    ASSERT(xv_queue_size(queue) == 5);

    ASSERT(xv_queue_pop(queue) == (void *)1);
    ASSERT(xv_queue_pop(queue) == (void *)2);
    ASSERT(xv_queue_pop(queue) == (void *)3);

    ASSERT(xv_queue_size(queue) == 2);

    ASSERT(xv_queue_pop(queue) == (void *)4);
    ASSERT(xv_queue_pop(queue) == (void *)5);

    ASSERT(xv_queue_size(queue) == 0);

    xv_queue_destroy(queue, NULL);

    // ----------------------------------------

    xv_concurrent_queue_t *concurrent_queue = xv_concurrent_queue_init();
    ASSERT(concurrent_queue != NULL);
    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 0);

    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == NULL);
    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == NULL);
    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == NULL);

    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 0);

    xv_concurrent_queue_push(concurrent_queue, (void *)1);

    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 1);

    xv_concurrent_queue_push(concurrent_queue, (void *)2);
    xv_concurrent_queue_push(concurrent_queue, (void *)3);
    xv_concurrent_queue_push(concurrent_queue, (void *)4);
    xv_concurrent_queue_push(concurrent_queue, (void *)5);

    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 5);

    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == (void *)1);
    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == (void *)2);
    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == (void *)3);

    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 2);

    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == (void *)4);
    ASSERT(xv_concurrent_queue_pop(concurrent_queue) == (void *)5);

    ASSERT(xv_concurrent_queue_size(concurrent_queue) == 0);

    xv_concurrent_queue_destroy(concurrent_queue, NULL);

    return EXIT_SUCCESS;
}

