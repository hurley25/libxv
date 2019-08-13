/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_loop_async_test.c 08/04/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "xv_test.h"

#define SEND_STR "Hello libxv!"

void *async_send_fun(void *args)
{
    xv_async_t *async = (xv_async_t *)args;

    int ret = xv_async_send(async);
    ASSERT(ret == XV_OK);

    // event will merge, so we need sleep
    usleep(10000);

    ret = xv_async_send(async);
    ASSERT(ret == XV_OK);

    usleep(10000);

    ret = xv_async_send(async);
    ASSERT(ret == XV_OK);

    return NULL;
}

void async_cb(xv_loop_t *loop, xv_async_t *async)
{
    ASSERT(xv_async_get_userdata(async) == loop);

    static int count = 0;
    count++;
    fprintf(stderr, "No.%d %s\n", count, SEND_STR);

    if (count == 3) {
        xv_loop_break(loop);
    }
}

int main(int argc, char *argv[])
{
    //xv_set_log_level(XV_LOG_DEBUG);

    xv_loop_t *loop = xv_loop_init(1024);

    xv_async_t *async = xv_async_init(async_cb);
    xv_async_set_userdata(async, loop);
    ASSERT(async != NULL);

    int ret = xv_async_start(loop, async);
    ASSERT(ret == XV_OK);

    pthread_t id;
    ret = pthread_create(&id, NULL, async_send_fun, async);
    CHECK(ret == 0, "pthread_create: ");

    // blockiong here
    xv_loop_run(loop);

    ret = pthread_join(id, NULL);
    CHECK(ret == 0, "pthread_join: ");

    ret = xv_async_stop(loop, async);
    ASSERT(ret == XV_OK);
    ret = xv_async_destroy(async);
    ASSERT(ret == XV_OK);

    xv_loop_destroy(loop);

    return EXIT_SUCCESS;
}

