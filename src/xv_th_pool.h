/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_th_pool.h 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_THREAD_POOL_H_
#define XV_THREAD_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------------------
// xv_worker_thread_t
// ----------------------------------------------------------------------------------------
typedef struct xv_worker_thread_t xv_worker_thread_t;

xv_worker_thread_t *xv_worker_thread_init(void);
void xv_worker_thread_destroy(xv_worker_thread_t *thread);
int xv_worker_thread_start(xv_worker_thread_t *thread);
int xv_worker_thread_stop(xv_worker_thread_t *thread);
int xv_worker_thread_push_task(xv_worker_thread_t *pool, void (*cb)(void *), void *args);
int xv_worker_thread_task_count(xv_worker_thread_t *thread);

// ----------------------------------------------------------------------------------------
// xv_thread_pool_t
// ----------------------------------------------------------------------------------------
typedef struct xv_thread_pool_t xv_thread_pool_t;

xv_thread_pool_t *xv_thread_pool_init(int thread_count);
void xv_thread_pool_destroy(xv_thread_pool_t *pool);
int xv_thread_pool_start(xv_thread_pool_t *pool);
int  xv_thread_pool_stop(xv_thread_pool_t *pool);
int xv_thread_pool_push_task(xv_thread_pool_t *pool, void (*cb)(void *), void *args, int hashcode);
int xv_thread_pool_task_count(xv_thread_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif // XV_THREAD_POOL_H_

