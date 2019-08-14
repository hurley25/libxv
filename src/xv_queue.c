/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_queue.c 08/11/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_queue.h"

#include <stdlib.h>
#include <pthread.h>

// ----------------------------------------------------------------------------------------
// xv_queue_t
// ----------------------------------------------------------------------------------------
typedef struct xv_node_t {
    void *data;
    struct xv_node_t *next;
} xv_node_t;

struct xv_queue_t {
    xv_node_t *head;
    xv_node_t *tail;
    int size;
};

xv_queue_t *xv_queue_init(void)
{
    xv_queue_t *queue = (xv_queue_t *)xv_malloc(sizeof(xv_queue_t));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

void xv_queue_destroy(xv_queue_t *queue, xv_queue_data_destroy_cb_t destroy)
{
    xv_node_t *node = queue->head;
    while (node != NULL) {
        xv_node_t *tmp = node->next;
        if (destroy) {
            destroy(node->data);
        }
        xv_free(node);
        node = tmp;
    }
    xv_free(queue);
}

void xv_queue_push(xv_queue_t *queue, void *data)
{
    xv_node_t *node = (xv_node_t *)xv_malloc(sizeof(xv_node_t));
    node->data = data;
    if (queue->tail) {
        queue->tail->next = node;
        queue->tail = node;
    } else {
        queue->head = queue->tail = node;
    }
    queue->size++;
}

void *xv_queue_pop(xv_queue_t *queue)
{
    void *data = NULL;
    if (queue->size == 1) {
        data = queue->head->data;
        xv_free(queue->head);
        queue->head = queue->tail = NULL;
        queue->size--;
    } else if (queue->size > 1) {
        data = queue->head->data;
        xv_node_t *tmp = queue->head;
        queue->head = queue->head->next;
        xv_free(tmp);
        queue->size--;
    }

    return data;
}

int xv_queue_size(xv_queue_t *queue)
{
    return queue->size;
}

// ----------------------------------------------------------------------------------------
// xv_concurrent_queue_t
// ----------------------------------------------------------------------------------------
struct xv_concurrent_queue_t {
    xv_queue_t *queue;
    pthread_mutex_t mutex;
};

xv_concurrent_queue_t *xv_concurrent_queue_init(void)
{
    xv_concurrent_queue_t *queue = (xv_concurrent_queue_t *)xv_malloc(sizeof(xv_concurrent_queue_t));
    queue->queue = xv_queue_init();
    pthread_mutex_init(&queue->mutex, NULL);

    return queue;
}

void xv_concurrent_queue_destroy(xv_concurrent_queue_t *queue, xv_queue_data_destroy_cb_t destroy)
{
    xv_queue_destroy(queue->queue, destroy);
    pthread_mutex_destroy(&queue->mutex);
    xv_free(queue);
}

void xv_concurrent_queue_push(xv_concurrent_queue_t *queue, void *data)
{
    pthread_mutex_lock(&queue->mutex);
    xv_queue_push(queue->queue, data);
    pthread_mutex_unlock(&queue->mutex);
}

void *xv_concurrent_queue_pop(xv_concurrent_queue_t *queue)
{
    pthread_mutex_lock(&queue->mutex);
    void *data = xv_queue_pop(queue->queue);
    pthread_mutex_unlock(&queue->mutex);

    return data;
}

int xv_concurrent_queue_size(xv_concurrent_queue_t *queue)
{
    pthread_mutex_lock(&queue->mutex);
    int size = xv_queue_size(queue->queue);
    pthread_mutex_unlock(&queue->mutex);

    return size;
}

