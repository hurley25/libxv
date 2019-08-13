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
#include "xv_buffer.h"
#include "xv_socket.h"
#include "xv_thread_pool.h"

#define XV_DEFAULT_LOOP_SIZE 1024
#define XV_DEFAULT_BUFFRT_SIZE 8192
#define XV_DEFAULT_READ_SIZE 4096

// ----------------------------------------------------------------------------------------
// xv_connection_t
// ----------------------------------------------------------------------------------------
static xv_connection_t *xv_connection_init(const char *addr, int port, int fd, 
                   xv_server_handle_t *handle, xv_io_cb_t read_cb, xv_io_cb_t write_cb)
{
    xv_connection_t *conn = (xv_connection_t *)xv_malloc(sizeof(xv_connection_t));
    strncpy(conn->addr, addr, XV_ADDR_LEN);
    conn->port = port;
    conn->fd = fd;
    conn->handle = handle;
    conn->io_thread = NULL;

    conn->read_io = xv_io_init(fd, XV_READ, read_cb);
    xv_io_set_userdata(conn->read_io, conn);

    conn->write_io = xv_io_init(fd, XV_WRITE, write_cb);
    xv_io_set_userdata(conn->write_io, conn);

    conn->read_buffer = xv_buffer_init(XV_DEFAULT_BUFFRT_SIZE);
    conn->write_buffer = xv_buffer_init(XV_DEFAULT_BUFFRT_SIZE);

    return conn;
}

static void xv_connection_stop(xv_loop_t *loop, xv_connection_t *conn)
{
    xv_io_stop(loop, conn->read_io);
    xv_io_stop(loop, conn->write_io);
    xv_close(conn->fd);
}

static void xv_connection_destroy(xv_connection_t *conn)
{
    xv_io_destroy(conn->read_io);
    xv_io_destroy(conn->write_io);
    xv_buffer_destroy(conn->read_buffer);
    xv_buffer_destroy(conn->write_buffer);
    xv_free(conn);
}

// ----------------------------------------------------------------------------------------
// xv_listener_t
// ----------------------------------------------------------------------------------------
struct xv_listener_t {
    char addr[XV_ADDR_LEN];        // bind addr
    int port;                      // listen port
    int listen_fd;
    xv_io_t *listen_io;            // listen_fd readable cb
    xv_server_handle_t handle;     // user cb handle
    xv_io_thread_t *io_thread;     // which io thread call `xv_io_start`

    xv_listener_t *next;
};

static xv_listener_t *xv_listener_init(const char *addr, int port, int fd, xv_server_handle_t handle, xv_io_cb_t new_conn_cb)
{
    xv_listener_t *listener = (xv_listener_t *)xv_malloc(sizeof(xv_listener_t));

    strncpy(listener->addr, addr, XV_ADDR_LEN);
    listener->port = port;
    listener->listen_fd = fd;
    listener->listen_io = xv_io_init(fd, XV_READ, new_conn_cb);
    listener->handle = handle;
    listener->io_thread = NULL;

    xv_io_set_userdata(listener->listen_io, listener);

    return listener;
}

static void xv_listener_stop(xv_loop_t *loop, xv_listener_t *listener)
{
    xv_io_stop(loop, listener->listen_io);
    xv_close(listener->listen_fd);
}

static void xv_listener_destroy(xv_listener_t *listener)
{
    xv_io_destroy(listener->listen_io);
    xv_free(listener);
}

// ----------------------------------------------------------------------------------------
// xv_message_t
// ----------------------------------------------------------------------------------------
xv_message_t *xv_message_init(xv_connection_t *conn, void *request)
{
    xv_message_t *message = (xv_message_t *)xv_malloc(sizeof(xv_message_t));
    message->conn = conn;
    message->request = request;
    message->response = NULL;

    return message;
}

void xv_message_destroy(xv_message_t *message, void (*packet_cleanup)(void *))
{
    if (packet_cleanup) {
        if (message->request) {
            packet_cleanup(message->request);
        }
        if (message->response) {
            packet_cleanup(message->response);
        }
    }
    xv_free(message);
}

// ----------------------------------------------------------------------------------------
// xv_io_thread_t
// ----------------------------------------------------------------------------------------
struct xv_io_thread_t {
    int idx;
    pthread_t id;
    xv_loop_t *loop;
    xv_server_t *server;
    int conn_setsize;
    xv_connection_t **connections;
};

static xv_io_thread_t *xv_io_thread_init(int i, xv_server_t *server)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)xv_malloc(sizeof(xv_io_thread_t));
    io_thread->idx = i;
    io_thread->loop = xv_loop_init(XV_DEFAULT_LOOP_SIZE);
    io_thread->server = server;

    int conn_array_size = sizeof(xv_connection_t *) * XV_DEFAULT_LOOP_SIZE;
    io_thread->connections = (xv_connection_t **)xv_malloc(conn_array_size);
    memset(io_thread->connections, 0, conn_array_size);

    io_thread->conn_setsize = XV_DEFAULT_LOOP_SIZE;

    return io_thread;
}

static void xv_io_thread_stop(xv_io_thread_t *io_thread)
{
    // stop all connection
    for (int i = 0; i < io_thread->conn_setsize; ++i) {
        if (io_thread->connections[i]) {
            xv_connection_stop(io_thread->loop, io_thread->connections[i]);
        }
    }
    // break loop
    xv_loop_break(io_thread->loop);
}

static void xv_io_thread_add_connection(xv_io_thread_t *io_thread, xv_connection_t *conn)
{
    if (conn->fd > io_thread->conn_setsize) {
        io_thread->conn_setsize *= 2;
        int conn_array_size = sizeof(xv_connection_t *) * io_thread->conn_setsize;
        io_thread->connections = (xv_connection_t **)xv_realloc(io_thread->connections, conn_array_size);
    }
    io_thread->connections[conn->fd] = conn;
}

static void xv_io_thread_del_connection(xv_io_thread_t *io_thread, xv_connection_t *conn)
{
    io_thread->connections[conn->fd] = NULL;
}

static void xv_io_thread_destroy(xv_io_thread_t *io_thread)
{
    // destory all connection
    for (int i = 0; i < io_thread->conn_setsize; ++i) {
        if (io_thread->connections[i]) {
            xv_connection_destroy(io_thread->connections[i]);
        }
    }
    xv_free(io_thread->connections);
    xv_loop_destroy(io_thread->loop);
    xv_free(io_thread);
}

// ----------------------------------------------------------------------------------------
// xv_server_t
// ----------------------------------------------------------------------------------------
struct xv_server_t {
    xv_server_config_t config;
    xv_io_thread_t **io_threads;
    xv_thread_pool_t *worker_threads;
    xv_listener_t *listeners;
    int start;
};

static void xv_connection_close(xv_connection_t *conn)
{
    // call user on_disconnect
    if (conn->handle->on_disconnect) {
        conn->handle->on_disconnect(conn);
    }
    xv_io_thread_del_connection(conn->io_thread, conn);
    xv_connection_stop(conn->io_thread->loop, conn);
    xv_connection_destroy(conn);   
}

static void process_read_buffer(xv_loop_t *loop, xv_connection_t *conn, xv_server_handle_t *handle)
{
    // do user decode
    if (!handle->decode || !handle->process) {
        // clear buffer, drop data and return
        xv_buffer_clear(conn->read_buffer);
        return;
    }
    void *request;
    int ret = handle->decode(conn->read_buffer, &request);
    if (ret == XV_OK) {
        //  do user process
        xv_message_t *message = xv_message_init(conn, request);
        handle->process(message);
        if (message->response && handle->encode) {
            handle->encode(conn->write_buffer, message->response);
            int buffer_size = xv_buffer_readable_size(conn->write_buffer);
            if (buffer_size > 0) {
                int nwritten = xv_write(conn->fd, xv_buffer_read_begin(conn->write_buffer), buffer_size);
                if (nwritten == 0 || (nwritten == -1 && errno != EAGAIN)) {
                    xv_connection_close(conn);
                }
                // incr buffer index
                xv_buffer_incr_read_index(conn->write_buffer, nwritten);
                if (nwritten != buffer_size) {
                    // unhappy, kernel socket buffer is full, start write event
                    xv_io_start(loop, conn->write_io);
                }
            }
        }
        xv_message_destroy(message, handle->packet_cleanup);
    } else if (ret == XV_ERR) {
        // decode failed! close it
        xv_connection_close(conn);
    }
    // else XV_AGAIN, do nothing...
}

static void on_connection_read(xv_loop_t *loop, xv_io_t *io)
{
    int fd = xv_io_get_fd(io);
    xv_connection_t *conn = (xv_connection_t *)xv_io_get_userdata(io);
    xv_server_handle_t *handle = conn->handle;

    xv_buffer_ensure_writeable_size(conn->read_buffer, XV_DEFAULT_READ_SIZE);

    int nread = xv_read(fd, xv_buffer_write_begin(conn->read_buffer), XV_DEFAULT_READ_SIZE);
    if (nread <= 0) {
        if (nread == -1 && errno == EAGAIN) {
            return;
        }
        // will close it
        xv_connection_close(conn);
    } else {
        // ret > 0, incr buffer index
        xv_buffer_incr_write_index(conn->read_buffer, nread);
        process_read_buffer(loop, conn, handle);
    }
}

static void on_connection_write(xv_loop_t *loop, xv_io_t *io)
{
    xv_connection_t *conn = (xv_connection_t *)xv_io_get_userdata(io);

    int buffer_size = xv_buffer_readable_size(conn->write_buffer);
    if (buffer_size > 0) {
        int nwritten = xv_write(conn->fd, xv_buffer_read_begin(conn->write_buffer), buffer_size);
        if (nwritten == 0 || (nwritten == -1 && errno != EAGAIN)) {
            xv_connection_close(conn);
        }
        // incr buffer index
        xv_buffer_incr_read_index(conn->write_buffer, nwritten);
        if (nwritten == buffer_size) {
            // happy, write all data success, stop write event
            xv_io_stop(loop, conn->write_io);
        }
    } else {
        xv_io_stop(loop, conn->write_io);
    }
}

static void on_new_connection(xv_loop_t *loop, xv_io_t *io)
{
    int listen_fd = xv_io_get_fd(io);

    char addr[XV_ADDR_LEN];
    int port;
    int client_fd = xv_tcp_accept(listen_fd, addr, sizeof(addr), &port);
    if (client_fd > 0) {
        xv_log_debug("xv_tcp_accept new connection: %s:%d", addr, port);

        xv_listener_t *listener = (xv_listener_t *)xv_io_get_userdata(io);
        xv_server_handle_t *handle = &listener->handle;
        xv_connection_t *conn = xv_connection_init(addr, port, client_fd, handle, on_connection_read, on_connection_write);

        // user on_conn callback
        if (handle->on_connect) {
            handle->on_connect(conn);
        }

        xv_server_config_t *config = &(listener->io_thread->server->config);
        // add conn to myself conn list or send conn to other io thread
        if (config->io_thread_count == 1) {
            xv_io_thread_t *io_thread = listener->io_thread;
            conn->io_thread = io_thread;

            // add conn to io_thread
            xv_io_thread_add_connection(io_thread, conn);
            xv_io_start(loop, conn->read_io);
        } else {
            // TODO
        }
    }
}

static void *io_thread_entry(void *args)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)args;
    xv_server_t *server = io_thread->server;
    
    if (io_thread->idx == 0) {
        xv_log_debug("I'am leader IO Thread, add all listen fd event");

        xv_listener_t *listener = server->listeners;
        while (listener) {
            xv_log_debug("leader IO Thread add listener, addr: %s:%d", listener->addr, listener->port);

            listener->io_thread = io_thread;
            xv_io_start(io_thread->loop, listener->listen_io);
            listener = listener->next;
        }
    } else {
        xv_log_debug("I'am follower IO Thread No.%d, wait Leader send xv_connection_t", io_thread->idx);
    }

    // loop run until server stop
    xv_loop_run_timeout(io_thread->loop, 10);  // 100 times per second

    if (io_thread->idx == 0) {
        xv_log_debug("I'am leader IO Thread, del all listen fd evebt");

        xv_listener_t *listener = server->listeners;
        while (listener) {
            xv_log_debug("leader IO Thread del listener, addr: %s:%d", listener->addr, listener->port);
            xv_io_stop(io_thread->loop, listener->listen_io);
            listener->io_thread = NULL;
            listener = listener->next;
        }
        xv_log_debug("leader IO Thread exit");

    } else {
        xv_log_debug("follower IO Thread exit");
    }

    return NULL;
}

xv_server_t *xv_server_init(xv_server_config_t config)
{
    if (config.io_thread_count <= 0 || config.worker_thread_count < 0) {
        xv_log_error("config.io_thread_count must > 0, config.worker_thread_count must >= 0");
        return NULL;
    }
    xv_server_t *server = (xv_server_t *)xv_malloc(sizeof(xv_server_t));
    server->io_threads = (xv_io_thread_t **)xv_malloc(sizeof(xv_io_thread_t *) * config.io_thread_count);
    for (int i = 0; i < config.io_thread_count; ++i) {
        server->io_threads[i] = xv_io_thread_init(i, server);
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

int xv_server_add_listen(xv_server_t *server, const char *addr, int port, xv_server_handle_t handle)
{
    int listen_fd = xv_tcp_listen(addr, port, 1024);
    if (listen_fd < 0) {
        xv_log_error("listen on %s:%d failed!");
        return XV_ERR;
    }

    xv_listener_t *listener = xv_listener_init(addr, port, listen_fd, handle, on_new_connection);

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
    server->start = 1;
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        int ret = pthread_create(&server->io_threads[i]->id, NULL, io_thread_entry, server);
        if (ret != 0) {
            xv_log_errno_error("pthread_create io thread failed!");
            return XV_ERR;
        }
    }

    return XV_OK;
}

int xv_server_run(xv_server_t *server)
{
    if (!server->start) {
        xv_log_error("server is not started!");
        return XV_ERR;
    }
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        int ret = pthread_join(server->io_threads[i]->id, NULL);
        if (ret != 0) {
            xv_log_errno_error("pthread_join io thread failed!");
            return XV_ERR;
        }
    }
    
    return XV_OK;
}

int xv_server_stop(xv_server_t *server)
{
    if (server->start == 0) {
        return XV_ERR;
    }
    server->start = 0;
    xv_memory_barriers();

    // stop all listeners
    xv_listener_t *listener = server->listeners;
    while (listener) {
        xv_listener_stop(listener->io_thread->loop, listener);
        listener = listener->next;
    }

    // stop all connections
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        xv_io_thread_stop(server->io_threads[i]);
    }

    // stop worker thread pool
    if (server->worker_threads) {
        xv_thread_pool_stop(server->worker_threads);
    }

    return XV_OK;
}

void xv_server_destroy(xv_server_t *server)
{
    xv_server_stop(server);

    // destroy all listeners
    xv_listener_t *listener = server->listeners;
    while (listener) {
        xv_listener_t *tmp = listener->next;
        xv_listener_destroy(listener);
        listener = tmp;
    }

    // destroy all io thread
    for (int i = 0; i < server->config.io_thread_count; ++i) {
        xv_io_thread_destroy(server->io_threads[i]);
    }

    // destroy worker thread pool
    if (server->worker_threads) {
        xv_thread_pool_destroy(server->worker_threads);
    }
}

