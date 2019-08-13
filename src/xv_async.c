/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv.c 2019/08/03 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv.h"
#include "xv_log.h"

#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
    #include <sys/eventfd.h>
#else
    // use pipe(2) of unistd.h
#endif

// ----------------------------------------------------------------------------------------
// xv_async
// ----------------------------------------------------------------------------------------

struct xv_async_t {
#ifdef __linux__
    int evfd;
#else
    int pipefd[2];
#endif
    xv_async_cb_t cb;
    void *userdata;
    xv_io_t *read_io;
};

void xv_async_set_userdata(xv_async_t *async, void *data)
{
    async->userdata = data;
}

void *xv_async_get_userdata(xv_async_t *async)
{
    return async->userdata;
}

static void xv_async_cb(xv_loop_t *loop, xv_io_t *io)
{
#ifdef __linux__
    eventfd_t num = 0;
    int ret = eventfd_read(xv_io_get_fd(io), &num);
    if (ret < 0) {
        xv_log_errno_error("eventfd_read");
    }
    // do not care this num value, just log debug
    xv_log_debug("xv_async_cb read num: %llu", num);
#else
    // TODO
#endif

    xv_async_t *async = (xv_async_t *)xv_io_get_userdata(io);
    if (async->cb) {
        async->cb(loop, async);
    }
}

xv_async_t *xv_async_init(xv_async_cb_t cb)
{
    if (!cb) {
        xv_log_error("async cb is NULL!");
        return NULL;
    }

    xv_async_t *async = (xv_async_t *)xv_malloc(sizeof(xv_async_t));

#ifdef __linux__
    // `EFD_NONBLOCK` since Linux 2.6.27
    int evfd = eventfd(0, EFD_NONBLOCK);
    if (evfd < 0) {
        xv_log_errno_error("eventfd failed");
        xv_free(async);
        return NULL;
    }

    xv_log_debug("async init create eventfd is %d", evfd);

    async->evfd = evfd;
    async->read_io = xv_io_init(async->evfd, XV_READ, xv_async_cb);
#else
    // TODO
#endif

    async->cb = cb;
    async->userdata = NULL;
    xv_io_set_userdata(async->read_io, async);

    return async;
}

int xv_async_start(xv_loop_t *loop, xv_async_t *async)
{
    xv_log_debug("async_t start, evfd: %d", async->evfd);

    return xv_io_start(loop, async->read_io);
}

int xv_async_stop(xv_loop_t *loop, xv_async_t *async)
{
    xv_log_debug("async_t stop, evfd: %d", async->evfd);

    return xv_io_stop(loop, async->read_io);
}

int xv_async_send(xv_async_t *async)
{
    xv_log_debug("eventfd_write to xv_async_t, evfd: %d", async->evfd);

#ifdef __linux__
    eventfd_t num = 1;
    if (eventfd_write(async->evfd, num) < 0) {
        xv_log_errno_error("eventfd_write failed");
        return XV_ERR;
    }
#else
    // TODO
#endif

    return XV_OK;
}

int xv_async_destroy(xv_async_t *async)
{
    xv_log_debug("async_t destroy, evfd: %d", async->evfd);
    int ret = xv_io_destroy(async->read_io);
    if (ret != XV_OK) {
        xv_log_error("async_t destroy failed!");
        return ret;
    }

#ifdef __linux__
    close(async->evfd);
#else
    // TODO
#endif
    xv_free(async);

    return XV_OK;
}

