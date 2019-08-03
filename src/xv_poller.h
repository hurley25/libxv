/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_poller.h 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_POLLER_H_
#define XV_POLLER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xv_fired_io {
    int fd;
    int event;
} xv_fired_event_t;

// ----------------------------------------------------------------------------------------
// poller interface
// ----------------------------------------------------------------------------------------

typedef struct xv_poller_data {
    int epfd;
    int setsize;
    struct epoll_event *events;
} xv_poller_data_t;

xv_poller_data_t *xv_poller_init(int setsize);
void xv_poller_destroy(xv_poller_data_t *data);
int xv_poller_resize(xv_poller_data_t *data, int setsize);
int xv_poller_add_event(xv_poller_data_t *data, int fd, int old_event, int event);
int xv_poller_del_event(xv_poller_data_t *data, int fd, int old_event, int event);
int xv_poller_poll(xv_poller_data_t *data, xv_fired_event_t *fired_events, int timeout_ms);
const char *xv_poller_name(void);

#ifdef __cplusplus
}
#endif

#endif // XV_POLLER_H_

