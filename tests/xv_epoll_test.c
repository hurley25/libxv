/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_epoll_test.c 2019/08/09 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "xv_test.h"
#include "xv_poller.h"

int main(int argc, char *argv[])
{
    xv_set_log_level(XV_LOG_DEBUG);

    xv_poller_data_t *data = xv_poller_init(1024);
    ASSERT(data != NULL);
    ASSERT(data->epfd >= 0);
    ASSERT(data->events != NULL);
    ASSERT(data->setsize == 1024);

    xv_poller_resize(data, 2048);
    ASSERT(data->setsize == 2048);

    // add read
    int ret = xv_poller_add_event(data, STDOUT_FILENO, XV_NONE, XV_READ);
    CHECK(ret == XV_OK, "xv_poller_add_event failed: ");

    // add write
    ret = xv_poller_add_event(data, STDOUT_FILENO, XV_READ, XV_WRITE);
    CHECK(ret == XV_OK, "xv_poller_add_event failed: ");

    xv_fired_event_t events[128];
    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 1, "xv_poller_poll return not 1: ");
    ASSERT(events[0].fd == STDOUT_FILENO);
    ASSERT(events[0].event == XV_WRITE);

    // del read
    ret = xv_poller_del_event(data, STDOUT_FILENO, XV_ALL_EVENT, XV_READ);
    CHECK(ret == XV_OK, "xv_poller_del_event failed: ");

    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 1, "xv_poller_poll return not 1: ");
    ASSERT(events[0].fd == STDOUT_FILENO);
    ASSERT(events[0].event == XV_WRITE);

    // del write
    ret = xv_poller_del_event(data, STDOUT_FILENO, XV_WRITE, XV_WRITE);
    CHECK(ret == XV_OK, "xv_poller_del_event failed: ");

    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 0, "xv_poller_poll return not 0: ");

    // add write
    ret = xv_poller_add_event(data, STDOUT_FILENO, XV_NONE, XV_WRITE);
    CHECK(ret == XV_OK, "xv_poller_add_event failed: ");

    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 1, "xv_poller_poll return not 1: ");
    ASSERT(events[0].fd == STDOUT_FILENO);
    ASSERT(events[0].event == XV_WRITE);

    // add read
    ret = xv_poller_add_event(data, STDOUT_FILENO, XV_WRITE, XV_READ);
    CHECK(ret == XV_OK, "xv_poller_add_event failed: ");

    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 1, "xv_poller_poll return not 1: ");
    ASSERT(events[0].fd == STDOUT_FILENO);
    ASSERT(events[0].event == XV_WRITE);

    // del write
    ret = xv_poller_del_event(data, STDOUT_FILENO, XV_ALL_EVENT, XV_WRITE);
    CHECK(ret == XV_OK, "xv_poller_del_event failed: ");

    ret = xv_poller_poll(data, events, 0);
    CHECK(ret == 0, "xv_poller_poll return not 0: ");

    xv_poller_destroy(data);

    return EXIT_SUCCESS;
}

