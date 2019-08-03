/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_epoll.c 2019/08/04 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "xv_define.h"
#include "xv_poller.h"
#include "xv_log.h"

// for log
static const char *xv_epoll_op_to_str(int op)
{
    switch (op) {
        case EPOLL_CTL_ADD: // 1
            return "ADD";
        case EPOLL_CTL_DEL: // 2
            return "DEL";
        case EPOLL_CTL_MOD: // 3
            return "MOD";
        default:
            break;
    }

    return "ERROR_OP";
}

xv_poller_data_t *xv_poller_init(int setsize)
{
    xv_poller_data_t *data = (xv_poller_data_t *)xv_malloc(sizeof(xv_poller_data_t));
    data->epfd = epoll_create(setsize);
    data->events = (struct epoll_event *)xv_malloc(sizeof(struct epoll_event) * setsize);
    data->setsize = setsize;

    xv_log_debug("init epoll, fd id %d, setsize is %d", data->epfd, setsize);

    return data;
}

void xv_poller_destroy(xv_poller_data_t *data)
{
    xv_log_debug("destroy epoll, fd is %d, setsize is %d", data->epfd, data->setsize);

    xv_free(data->events);
    xv_free(data);
}

int xv_poller_resize(xv_poller_data_t *data, int setsize)
{
    xv_log_debug("epoll resize, setsize: %d -> %d", data->setsize, setsize);

    data->events = (struct epoll_event *)xv_realloc(data->events, sizeof(struct epoll_event) * setsize);
    data->setsize = setsize;

    return XV_OK;
}

static int xv_poller_modify_event(xv_poller_data_t *data, int fd, int mask, int op)
{
    xv_log_debug("epoll modify event, fd: %d, event: %s, op: %s ",
            fd, xv_event_to_str(mask), xv_epoll_op_to_str(op));

    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = 0;

    if (mask & XV_READ) {
        event.events |= EPOLLIN;
    }
    if (mask & XV_WRITE) {
        event.events |= EPOLLOUT;
    }

    int ret = epoll_ctl(data->epfd, op, fd, &event);
    if (ret != 0) {
        return XV_ERR;
    }

    return XV_OK;
}

int xv_poller_add_event(xv_poller_data_t *data, int fd, int old_event, int event)
{
    xv_log_debug("epoll add event, fd: %d, event: %s, old_event: %s",
            fd, xv_event_to_str(event), xv_event_to_str(old_event));

    int op = EPOLL_CTL_ADD;
    if (old_event != XV_NONE) {
        op = EPOLL_CTL_MOD;
        event |= old_event;
    }
    return xv_poller_modify_event(data, fd, event, op);
}

int xv_poller_del_event(xv_poller_data_t *data, int fd, int old_event, int event)
{
    xv_log_debug("epoll del event, fd: %d, event: %s, old_event: %s",
            fd, xv_event_to_str(event), xv_event_to_str(old_event));

    int op = EPOLL_CTL_DEL;
    int mask = old_event & (~event);
    if (mask != XV_NONE) {
        op = EPOLL_CTL_MOD;
        event = mask;
    }
    return xv_poller_modify_event(data, fd, event, op);
}

int xv_poller_poll(xv_poller_data_t *data, xv_fired_event_t *fired_events, int timeout_ms)
{
    xv_log_debug("will call epoll_wait, timeout_ms: %d", timeout_ms);

    int count = epoll_wait(data->epfd, data->events, data->setsize, timeout_ms);
    for (int i = 0; i < count; ++i) {
        // set fired fd
        fired_events[i].fd = data->events[i].data.fd;

        // clear old fired event
        fired_events[i].event = XV_NONE;

        // set fired events
        int events = data->events[i].events;
        if (events & EPOLLIN) {
            fired_events[i].event |= XV_READ;
        }
        if (events & EPOLLOUT) {
            fired_events[i].event |= XV_WRITE;
        }
        if (events & EPOLLHUP) {
            fired_events[i].event |= XV_WRITE;
        }
        if (events & EPOLLERR) {
            fired_events[i].event |= XV_WRITE;
        }
    }
    if (count < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            xv_log_errno_error("epoll_wait failed");
        }
    }

    xv_log_debug("epoll_wait return: %d", count);

    return count;
}

const char *xv_poller_name(void) {
    return "epoll";
}

