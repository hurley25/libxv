/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_th_pool.c 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_th_pool.h"

#include <stdlib.h>
#include <pthread.h>

#include "xv.h"
#include "xv_log.h"
#include "xv_queue.h"

// ----------------------------------------------------------------------------------------
// xv_worker_thread_t
// ----------------------------------------------------------------------------------------
struct xv_worker_thread_t {
    xv_concurrent_queue_t *task_queue;
    xv_loop_t *loop;
    xv_async_t *async;
    pthread_t id;
    int start;
};

typedef struct xv_task_t {
    void (*cb)(void *);
    void *args;
} xv_task_t;

static void worker_async_cb(xv_loop_t *loop, xv_async_t *async)
{
    xv_log_debug("worker thread run worker_async_cb");

    xv_worker_thread_t *thread = (xv_worker_thread_t *)xv_async_get_userdata(async);
    while (xv_concurrent_queue_size(thread->task_queue) > 0) {
        if (xv_curr_log_level == XV_LOG_DEBUG) {
            xv_log_debug("worker thread running task, task count: %d", xv_concurrent_queue_size(thread->task_queue));
        }
        xv_task_t *task = (xv_task_t *)xv_concurrent_queue_pop(thread->task_queue);
        if (task && task->cb) {
            task->cb(task->args);
            xv_free(task);
        }
    }
    if (!thread->start) {
        xv_log_debug("worker thread stopped, break loop");
        xv_loop_break(loop);
    }
}

static void *worker_entry(void *args)
{
    xv_worker_thread_t *thread = (xv_worker_thread_t *)args;
    while (thread->start) {
        xv_log_debug("thread->start is %d, will call xv_loop_run", thread->start);
        xv_loop_run(thread->loop);
    }
    xv_log_debug("loop break, worker thread exit");

    return NULL;
}

xv_worker_thread_t *xv_worker_thread_init(void)
{
    xv_log_debug("worker thread init");

    xv_worker_thread_t *thread = (xv_worker_thread_t *)xv_malloc(sizeof(xv_worker_thread_t));
    thread->task_queue = xv_concurrent_queue_init();
    thread->loop = xv_loop_init(1024);
    thread->async = xv_async_init(worker_async_cb);
    xv_async_set_userdata(thread->async, thread);
    thread->start = 0;

    return thread;
}

void xv_worker_thread_destroy(xv_worker_thread_t *thread)
{
    xv_log_debug("worker thread will destroy");

    xv_worker_thread_stop(thread);
    xv_async_send(thread->async);

    int ret = pthread_join(thread->id, NULL);
    if (ret != 0) {
        xv_log_errno_error("pthread_join");
    }

    xv_async_stop(thread->loop, thread->async);
    xv_async_destroy(thread->async);
    xv_loop_destroy(thread->loop);
    xv_concurrent_queue_destroy(thread->task_queue, xv_free);
    xv_free(thread);
}

int xv_worker_thread_start(xv_worker_thread_t *thread)
{
    if (thread->start) {
        xv_log_warn("worker thread already started!");
        return XV_ERR;
    }
    thread->start = 1;
    xv_memory_barriers();

    int ret = xv_async_start(thread->loop, thread->async);
    if (ret != XV_OK) {
        xv_log_error("worker thread xv_async_start failed!");
        return XV_ERR;
    }
    ret = pthread_create(&thread->id, NULL, worker_entry, thread);
    if (ret != 0) {
        xv_log_errno_error("pthread_create");
        return XV_ERR;
    }

    xv_log_debug("worker thread start");

    return XV_OK;
}

int xv_worker_thread_stop(xv_worker_thread_t *thread)
{
    thread->start = 0;
    xv_memory_barriers();

    xv_log_debug("worker thread stop");

    return XV_OK;
}

int xv_worker_thread_push_task(xv_worker_thread_t *thread, void (*cb)(void *), void *args)
{
    xv_task_t *task = (xv_task_t *)xv_malloc(sizeof(xv_task_t));
    task->cb = cb;
    task->args = args;
    xv_concurrent_queue_push(thread->task_queue, task);
    xv_async_send(thread->async);

    xv_log_debug("worker thread push task: %p, args: %p, weak up worker thread", cb, args);

    return XV_OK;
}

int xv_worker_thread_task_count(xv_worker_thread_t *thread)
{
    return xv_concurrent_queue_size(thread->task_queue);
}

// ----------------------------------------------------------------------------------------
// xv_thread_pool_t
// ----------------------------------------------------------------------------------------
struct xv_thread_pool_t {
    xv_worker_thread_t **threads;
    int thread_count;
    int start;
};

xv_thread_pool_t *xv_thread_pool_init(int thread_count)
{
    xv_thread_pool_t *pool = (xv_thread_pool_t *)xv_malloc(sizeof(xv_thread_pool_t));
    pool->threads = (xv_worker_thread_t **)xv_malloc(sizeof(xv_worker_thread_t *) * thread_count);
    for (int i = 0; i < thread_count; ++i) {
        pool->threads[i] = xv_worker_thread_init();
    }
    pool->thread_count = thread_count;
    pool->start = 0;

    return pool;
}

void xv_thread_pool_destroy(xv_thread_pool_t *pool)
{
    xv_thread_pool_stop(pool);
    for (int i = 0; i < pool->thread_count; ++i) {
        xv_worker_thread_destroy(pool->threads[i]);
    }
    xv_free(pool->threads);
    xv_free(pool);

    xv_log_debug("task pool destroy");
}

int xv_thread_pool_start(xv_thread_pool_t *pool)
{
    if (pool->start) {
        xv_log_error("thread pool already started!");
        return XV_ERR;
    }
    for (int i = 0; i < pool->thread_count; ++i) {
        xv_worker_thread_start(pool->threads[i]);
    }

    xv_log_debug("task pool start");

    return XV_OK;
}

int xv_thread_pool_stop(xv_thread_pool_t *pool)
{
    for (int i = 0; i < pool->thread_count; ++i) {
        xv_worker_thread_stop(pool->threads[i]);
    }

    xv_log_debug("task pool stop");

    return XV_OK;
}

int xv_thread_pool_push_task(xv_thread_pool_t *pool, void (*cb)(void *), void *args, int hashcode)
{
    int index = hashcode % pool->thread_count;
    xv_log_debug("task push to worker thread index: %d, pool->thread_count: %d", index, pool->thread_count);
    return xv_worker_thread_push_task(pool->threads[index], cb, args);
}

int xv_thread_pool_task_count(xv_thread_pool_t *pool)
{
    int count = 0;
    for (int i = 0; i < pool->thread_count; ++i) {
        count += xv_worker_thread_task_count(pool->threads[i]);
    }

    return count;
}

