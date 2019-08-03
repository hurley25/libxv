/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_thread_pool_test.c 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xv_test.h"
#include "xv_thread_pool.h"

void sum(void *args) {
    // disable gcc warning
    (void)args;
    int i, count = 0;
    for (i = 1; i <= 100; ++i) {
        count += i;
    }
    printf("%d\n", count);
}

int main(int argc, char *argv[])
{
    //xv_set_log_level(XV_LOG_DEBUG);

    xv_thread_pool_t *pool = xv_thread_pool_init(4);
    
    int ret = xv_thread_pool_start(pool);
    ASSERT(ret == XV_OK);

    ret = xv_thread_pool_push_task(pool, sum, NULL, 1);
    ASSERT(ret == XV_OK);
    ret = xv_thread_pool_push_task(pool, sum, NULL, 0);
    ASSERT(ret == XV_OK);
    ret = xv_thread_pool_push_task(pool, sum, NULL, 3);
    ASSERT(ret == XV_OK);
    ret = xv_thread_pool_push_task(pool, sum, NULL, 2);
    ASSERT(ret == XV_OK);

    usleep(100000);
    
    xv_thread_pool_destroy(pool);

    return EXIT_SUCCESS;
}

