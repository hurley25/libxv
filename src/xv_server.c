/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_server.c 08/12/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_server.h"

#include <stdlib.h>
#include <pthread.h>

#include "xv.h"
#include "xv_log.h"
#include "xv_socket.h"
#include "xv_thread_pool.h"

#define ADDR_LEN 32

typedef struct xv_io_thread_t xv_io_thread_t;
typedef struct xv_listener_t xv_listener_t;
typedef struct xv_connection_t xv_connection_t;

struct xv_listener_t {
    char addr[ADDR_LEN];
    int port;
    int listen_fd;
    xv_io_t *listen_io;
    xv_listen_handle_t handle;
    xv_io_thread_t *io_thread;
    xv_listener_t *next;
};

struct xv_connection_t {
    char addr[ADDR_LEN];
    int port;
    int fd;
    xv_listener_t *listener;
    xv_io_t *read_io;
    xv_io_t *write_io;
    xv_connection_t *next;
};

struct xv_io_thread_t {
    int idx;
    pthread_t id;
    xv_loop_t *loop;
    xv_server_t *server;
    xv_connection_t *connections;
};

struct xv_server_t {
    xv_server_config_t config;
    xv_io_thread_t *io_threads;
    xv_thread_pool_t *worker_threads;
    xv_listener_t *listeners;
    int start;
};

static void on_conn_read(xv_io_t *io)
{
    int fd = xv_io_get_fd(io);
    xv_connection_t *conn = (xv_connection_t *)xv_io_get_userdata(io);

    // xv_buffer_t
    //xv_read(fd, buff, size)
    
    xv_listen_handle_t *handle = *(conn->listener->handle);
    if (handle->decode) {
        //void *packet;
        //int ret = handle->decode()
        //if (ret == XV_OK) {
        //} else if (ret == XV_AGAIN) {
        //} else {
        //    // XV_AGAIN
        //    return;
        //}
    }
}

static void on_conn_write(xv_io_t *io)
{
    (void)io;
}

static void on_new_conn(xv_io_t *io)
{
    int listen_fd = xv_io_get_fd(io);
    xv_loop_t *loop = xv_io_get_loop(io);

    xv_connection_t *conn = (xv_connection_t *)xv_malloc(sizeof(xv_connection_t));
    int client_fd = xv_tcp_accept(listen_fd, conn->addr, sizeof(conn->addr), &conn->port);
    if (client_fd > 0) {
        xv_log_debug("xv_tcp_accept new connection: %s:%d", conn->addr, conn->port);

        conn->fd = client_fd;
        conn->listener = listener;

        // init read io & start
        conn->read_io = xv_io_init(client_fd, XV_READ, on_conn_read);
        xv_io_set_userdata(conn->read_io, conn);
        xv_io_start(loop, conn->read_io);

        // init write io but not start
        conn->write_io = xv_io_init(client_fd, XV_WRITE, on_conn_write);
        xv_io_set_userdata(conn->write_io, conn);

        xv_listener_t *listener = (xv_listener_t *)xv_io_get_userdata(io);

        // user on_conn callback
        if (listener->handle.on_connect) {
            listener->handle.on_connect(conn);
        }

        xv_server_config_t *config = &(listener->io_thread->server->config);
        if (config->io_thread_count == 1) {
            // add conn to myself 
            xv_io_thread_t *io_thread = listener->io_thread;
            conn->next = io_thread->connections;
            io_thread->connections = conn;
        } else {
            // send conn to other io thread
            // TODO
        }
    } else {
        xv_free(conn);
    }
}

static void *io_thread_entry(void *args)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)args;
    
    if (io_thread->idx == 0) {
        xv_log_debug("I'am leader IO Thread, add all listen fd event");
        xv_server_t *server = io_thread->server;
        xv_listener_t *listener = server->listeners;
        while (listener) {
            xv_log_debug("leader IO Thread add listener, addr: %s:%d", listener->addr, listener->port);
            xv_io_start(io_thread->loop, listener->listen_io);
            listener->io_thread = io_thread;
            listener = listener->next;
        }
    } else {
        xv_log_debug("I'am Follower IO Thread No.%d, wait Leader send xv_connection_t", io_thread->idx);
    }

    // loop run until server stop
    xv_loop_run_timeout(io_thread->loop, 10);  // 10ms loop once

    return NULL;
}

xv_server_t *xv_server_init(xv_server_config_t config)
{
    if (config.io_thread_count <= 0 || config.worker_thread_count < 0) {
        xv_log_error("config.io_thread_count must > 0, config.worker_thread_count must >= 0");
        return NULL;
    }
    xv_server_t *server = (xv_server_t *)xv_malloc(sizeof(xv_server_t));
    server->io_threads = (xv_io_thread_t *)xv_malloc(sizeof(xv_io_thread_t) * config.io_thread_count);
    for (int i = 0; i < config.io_thread_count; ++i) {
        server->io_threads[i].idx = i;
        server->io_threads[i].loop = xv_loop_init(1024);
        server->io_threads[i].server = server;
        server->io_threads[i].connections = NULL;
    }
    if (config.worker_thread_count == 0) {
        xv_log_error("not support worker thread now");
        server->worker_threads = NULL;
    } else {
        server->worker_threads = NULL;
    }
    server->listeners = NULL;
    server->start = 0;

    return NULL;
}

int xv_server_add_listen(xv_server_t *server, const char *addr, int port, xv_listen_handle_t handle)
{
    int listen_fd = xv_tcp_listen(addr, port, 1024);
    if (listen_fd < 0) {
        xv_log_error("listen on %s:%d failed!");
        return XV_ERR;
    }

    xv_listener_t *listener = (xv_listener_t *)xv_malloc(sizeof(xv_listener_t));
    strncpy(listener->addr, addr, ADDR_LEN - 1);
    listener->port = port;
    listener->handle = handle;
    xv_io_t *listen_io = xv_io_init(listen_fd, XV_READ, on_new_conn);
    xv_io_set_userdata(listen_io, listener);
    listener->listen_fd = listen_fd;

    // link to server->listeners's head
    listener->next = server->listeners;
    server->listeners = listener;

    return XV_OK;
}

int xv_server_start(xv_server_t *server)
{
    if (server->start) {
        xv_log_error("server already started!");
        return XV_ERR;
    }
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        int ret = pthread_create(&server->io_threads[i].id, NULL, io_thread_entry, server);
        if (ret != 0) {
            xv_log_errno_error("pthread_create io thread failed!");
            return XV_ERR;
        }
    }
    server->start = 1;

    return XV_OK;
}

int xv_server_run(xv_server_t *server)
{
    if (!server->start) {
        xv_log_error("server is not started!");
        return XV_ERR;
    }
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        int ret = pthread_join(server->io_threads[i].id, NULL);
        if (ret != 0) {
            xv_log_errno_error("pthread_join io thread failed!");
            return XV_ERR;
        }
    }
    
    return XV_OK;
}

int xv_server_stop(xv_server_t *server)
{
    server->start = 0;
    xv_memory_barriers();
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        xv_loop_break(server->io_threads[i].loop);
    }

    return XV_OK;
}

void xv_server_destroy(xv_server_t *server)
{
    xv_server_stop(server);
    // TODO
}

