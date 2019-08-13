/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv.h 2019/08/03 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_H_
#define XV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xv_define.h"

// ----------------------------------------------------------------------------------------
// xv_loop_t
// ----------------------------------------------------------------------------------------
typedef struct xv_loop_t xv_loop_t;

xv_loop_t *xv_loop_init(int setsize);
void xv_loop_run(xv_loop_t *loop);
void xv_loop_run_timeout(xv_loop_t *loop, int timeout_ms);
void xv_loop_run_once(xv_loop_t *loop);
void xv_loop_break(xv_loop_t *loop);
void xv_loop_destroy(xv_loop_t *loop);

// ----------------------------------------------------------------------------------------
// xv_io_t
// ----------------------------------------------------------------------------------------
typedef struct xv_io_t xv_io_t;
typedef void (*xv_io_cb_t)(xv_loop_t *loop, xv_io_t *);

int xv_io_get_fd(xv_io_t *io);

void xv_io_set_userdata(xv_io_t *io, void *data);
void *xv_io_get_userdata(xv_io_t *io);

xv_io_t *xv_io_init(int fd, int event, xv_io_cb_t cb);
int xv_io_start(xv_loop_t *loop, xv_io_t *io);
int xv_io_stop(xv_loop_t *loop, xv_io_t *io);
int xv_io_destroy(xv_io_t *io);

// ----------------------------------------------------------------------------------------
// xv_async_t
// ----------------------------------------------------------------------------------------
typedef struct xv_async_t xv_async_t;
typedef void (*xv_async_cb_t)(xv_loop_t *loop, xv_async_t *);

void xv_async_set_userdata(xv_async_t *async, void *data);
void *xv_async_get_userdata(xv_async_t *async);

xv_async_t *xv_async_init(xv_async_cb_t cb);
int xv_async_start(xv_loop_t *loop, xv_async_t *async);
int xv_async_stop(xv_loop_t *loop, xv_async_t *async);
int xv_async_destroy(xv_async_t *async);
int xv_async_send(xv_async_t *async);

// ----------------------------------------------------------------------------------------
// xv_timer_t
// ----------------------------------------------------------------------------------------
typedef struct xv_timer_t xv_timer_t;
typedef void (*xv_timer_cb_t)(xv_loop_t *loop, xv_timer_t *);

void xv_timer_set_userdata(xv_timer_t *timer, void *data);
void *xv_timer_get_userdata(xv_timer_t *timer);

xv_timer_t *xv_timer_init();
int xv_timer_start(xv_loop_t *loop, xv_timer_t *timer);
int xv_timer_stop(xv_loop_t *loop, xv_timer_t *timer);
int xv_timer_destroy(xv_timer_t *timer);

// ----------------------------------------------------------------------------------------
// xv_signal_t
// ----------------------------------------------------------------------------------------
typedef struct xv_signal_t xv_signal_t;
typedef void (*xv_signal_cb_t)(xv_loop_t *loop, xv_signal_t *);

void xv_signal_set_userdata(xv_signal_t *signal, void *data);
void *xv_signal_get_userdata(xv_signal_t *signal);

xv_signal_t *xv_signal_init();
int xv_signal_start(xv_loop_t *loop, xv_signal_t *signal);
int xv_signal_stop(xv_loop_t *loop, xv_signal_t *signal);
int xv_signal_destroy(xv_signal_t *signal);

#ifdef __cplusplus
}
#endif

#endif // XV_H_

