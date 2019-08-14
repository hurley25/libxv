/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_queue.h 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_QUEUE_H_
#define XV_QUEUE_H_

#include "xv_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*xv_queue_data_destroy_cb_t)(void *);

// ----------------------------------------------------------------------------------------
// xv_queue_t
// ----------------------------------------------------------------------------------------
typedef struct xv_queue_t xv_queue_t;

xv_queue_t *xv_queue_init(void);
void xv_queue_destroy(xv_queue_t *queue, xv_queue_data_destroy_cb_t destroy);
void xv_queue_push(xv_queue_t *queue, void *data);
void *xv_queue_pop(xv_queue_t *queue);
int xv_queue_size(xv_queue_t *queue);

// ----------------------------------------------------------------------------------------
// xv_concurrent_queue_t
// ----------------------------------------------------------------------------------------
typedef struct xv_concurrent_queue_t xv_concurrent_queue_t;

xv_concurrent_queue_t *xv_concurrent_queue_init(void);
void xv_concurrent_queue_destroy(xv_concurrent_queue_t *queue, xv_queue_data_destroy_cb_t destroy);
void xv_concurrent_queue_push(xv_concurrent_queue_t *queue, void *data);
void *xv_concurrent_queue_pop(xv_concurrent_queue_t *queue);
int xv_concurrent_queue_size(xv_concurrent_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif // XV_QUEUE_H_

