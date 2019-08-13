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
#include "xv_poller.h"
#include "xv_log.h"

#include <stdlib.h>
#include <unistd.h>

struct xv_io_t {
    int fd;
    int event;
    xv_io_cb_t cb;
    void *userdata;
    int start;
};

typedef struct xv_event_io_t {
    int event;
    xv_io_t *read_io;
    xv_io_t *write_io;
} xv_event_io_t;

struct xv_loop_t {
    xv_poller_data_t *poller_data;
    xv_event_io_t *events;
    xv_fired_event_t *fired_events;
    int setsize;
    int start;
};

xv_loop_t *xv_loop_init(int setsize)
{  
    xv_log_debug("loop init, setsize: %d", setsize);

    xv_loop_t *loop = (xv_loop_t *)xv_malloc(sizeof(xv_loop_t));
    loop->poller_data = xv_poller_init(setsize);

    loop->events = (xv_event_io_t *)xv_malloc(sizeof(xv_event_io_t) * setsize);
    for (int i = 0; i < setsize; ++i) {
        loop->events[i].event = XV_NONE;
        loop->events[i].read_io = NULL;
        loop->events[i].write_io = NULL;
    }
    loop->fired_events = (xv_fired_event_t *)xv_malloc(sizeof(xv_fired_event_t) * setsize);
    loop->setsize = setsize;
    loop->start = 1;

    return loop;
}

void xv_loop_destroy(xv_loop_t *loop)
{
    xv_log_debug("loop destroy, setsize: %d", loop->setsize);

    xv_poller_destroy(loop->poller_data);
    xv_free(loop->events);
    xv_free(loop->fired_events);
    xv_free(loop);
}

static void xv_loop_poll(xv_loop_t *loop, int timeout_ms)
{
    int count = xv_poller_poll(loop->poller_data, loop->fired_events, timeout_ms);
    for (int i = 0; i < count; ++i) {
        int fd = loop->fired_events[i].fd;
        int event = loop->fired_events[i].event;
        if (event & XV_READ) {
            xv_io_t *read_io = loop->events[fd].read_io;
            if (read_io && read_io->cb) {
                read_io->cb(loop, read_io);
            }
        }
        if (event & XV_WRITE) {
            xv_io_t *write_io = loop->events[fd].write_io;
            if (write_io && write_io->cb) {
                write_io->cb(loop, write_io);
            }
        }
    }
}

void xv_loop_run(xv_loop_t *loop)
{
    while (loop->start) {
        xv_loop_poll(loop, -1);
    }
}

void xv_loop_run_timeout(xv_loop_t *loop, int timeout_ms)
{
    while (loop->start) {
        xv_loop_poll(loop, timeout_ms);
    }
}

void xv_loop_run_once(xv_loop_t *loop)
{
    xv_loop_poll(loop, 0);
}

void xv_loop_break(xv_loop_t *loop)
{
    xv_log_debug("loop break");

    loop->start = 0;
    xv_memory_barriers();
}

static int xv_loop_resize(xv_loop_t *loop, int setsize)
{
    xv_log_debug("loop resize, setsize: %d -> %d", loop->setsize, setsize);

    if (setsize <= loop->setsize) {
        return XV_OK;
    }
    // resize poller
    xv_poller_resize(loop->poller_data, setsize);

    loop->events = (xv_event_io_t *)xv_realloc(loop->events, sizeof(xv_event_io_t) * setsize);
    for (int i = loop->setsize; i < setsize; ++i) {
        loop->events[i].event = XV_NONE;
        loop->events[i].read_io = NULL;
        loop->events[i].write_io = NULL;
    }
    loop->fired_events = (xv_fired_event_t *)xv_realloc(loop->events, sizeof(xv_fired_event_t) * setsize);
    loop->setsize = setsize;

    return XV_OK;
}

static int xv_loop_add_event(xv_loop_t *loop, xv_io_t *io)
{
    if (io->fd >= loop->setsize) {
        if (xv_loop_resize(loop, loop->setsize * 2) == XV_ERR) {
            xv_log_error("xv_loop_resize to %s failed", loop->setsize * 2);
            return XV_ERR;
        }
    }
    if (io->event != XV_READ && io->event != XV_WRITE) {
        xv_log_error("event must is XV_READ or XV_WRITE");
        return XV_ERR;
    }
    if (io->event & XV_READ) {
        loop->events[io->fd].read_io = io;
    }
    if (io->event & XV_WRITE) {
        loop->events[io->fd].write_io = io;
    }

    int old_event = loop->events[io->fd].event;
    loop->events[io->fd].event |= io->event;

    xv_log_debug("loop add event, fd: %d, event: %s, old_event %s, cb: %p, userdata: %p",
            io->fd, xv_event_to_str(io->event), xv_event_to_str(old_event), io->cb, io->userdata);

    if (xv_poller_add_event(loop->poller_data, io->fd, old_event, io->event) == XV_ERR) {
        return XV_ERR;
    }

    return XV_OK;
}

static int xv_loop_del_event(xv_loop_t *loop, xv_io_t *io)
{ 
    if (io->fd >= loop->setsize) {
        xv_log_error("fd[%d] not in loop record", io->fd);
        return XV_ERR;
    }
    if (io->event != XV_READ && io->event != XV_WRITE) {
        xv_log_error("event must is XV_READ or XV_WRITE");
        return XV_ERR;
    }
    int old_event = loop->events[io->fd].event;
    loop->events[io->fd].event &= (~io->event);

    if (io->event & XV_READ) {
        loop->events[io->fd].read_io = NULL;
    } else if (io->event & XV_WRITE) {
        loop->events[io->fd].write_io = NULL;
    }

    xv_log_debug("loop del event, fd: %d, event: %s, old_event: %s",
            io->fd, xv_event_to_str(io->event), xv_event_to_str(old_event));

    return xv_poller_del_event(loop->poller_data, io->fd, old_event, io->event);
}

// ----------------------------------------------------------------------------------------
// xv_io_t
// ----------------------------------------------------------------------------------------

int xv_io_get_fd(xv_io_t *io)
{
    return io->fd;
}

void xv_io_set_userdata(xv_io_t *io, void *data)
{
    io->userdata = data;
}

void *xv_io_get_userdata(xv_io_t *io)
{
    return io->userdata;
}

xv_io_t *xv_io_init(int fd, int event, xv_io_cb_t cb)
{
    if (!cb) {
        xv_log_error("io cb is NULL!");
        return NULL;
    }
    xv_io_t *io = (xv_io_t *)xv_malloc(sizeof(xv_io_t));
    io->fd = fd;
    io->event = event;
    io->cb = cb;
    io->userdata = NULL;
    io->start = 0;

    return io;
}

int xv_io_start(xv_loop_t *loop, xv_io_t *io)
{
    xv_log_debug("io_t start, fd: %d", io->fd);

    if (io->start) {
        xv_log_warn("xv_io_start failed, this xv_io_t already started!");
        return XV_ERR;
    }
    io->start = 1;
    return xv_loop_add_event(loop, io);
}

int xv_io_stop(xv_loop_t *loop, xv_io_t *io)
{
    xv_log_debug("io_t stop, fd: %d", io->fd);

    if (!io->start) {
        return XV_ERR;
    }
    io->start = 0;
    return xv_loop_del_event(loop, io);
}

int xv_io_destroy(xv_io_t *io)
{
    xv_log_debug("io_t destroy, fd: %d", io->fd);

    if (io->start) {
        xv_log_error("xv_io_t must stop before destroy!");  
        return XV_ERR;
    }
    xv_free(io);

    return XV_OK;
}

const char *xv_event_to_str(int event)
{
    switch (event) {
        case XV_NONE:
            return "NONE";
        case XV_READ:
            return "READ";
        case XV_WRITE:
            return "WRITE";
        case XV_ALL_EVENT:
            return "READ & WRITE";
        default:
            break;
    }

    return "ERROR_OP";
}

