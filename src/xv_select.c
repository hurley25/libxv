/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_select.c 2019/08/04 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv.h"
#include "xv_poller.h"

#include <stdlib.h>
#include <sys/select.h>

struct xv_poller_data {
    // ...
};

xv_poller_data_t *xv_poller_init(int setsize)
{
    return NULL;
}

void xv_poller_destroy(xv_poller_data_t *data)
{
}

int xv_poller_resize(xv_poller_data_t *data, int setsize)
{
    return XV_ERR;
}

int xv_poller_add_event(xv_poller_data_t *data, int fd, int old_event, int event)
{
    return XV_ERR;
}

int xv_poller_del_event(xv_poller_data_t *data, int fd, int old_event, int event)
{
    return XV_ERR;
}

int xv_poller_poll(xv_poller_data_t *data, xv_fired_event_t *fired_events, int timeout_ms)
{
    return XV_ERR;
}

const char *xv_poller_name(void)
{
    return "select";
}

