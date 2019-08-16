/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_service.c 08/12/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_service.h"

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "xv.h"
#include "xv_log.h"
#include "xv_queue.h"
#include "xv_buffer.h"
#include "xv_socket.h"
#include "xv_th_pool.h"

#define XV_DEFAULT_LOOP_SIZE 1024
#define XV_DEFAULT_BUFFRT_SIZE 8192
#define XV_DEFAULT_READ_SIZE 4096

// ----------------------------------------------------------------------------------------
// xv_connection_t
// ----------------------------------------------------------------------------------------
typedef enum xv_connection_status_t {
    XV_CONN_OPEN = 1,
    XV_CONN_CLOSED = 2,
} xv_connection_status_t;

typedef struct xv_connection_t {
    char addr[XV_ADDR_LEN];
    int port;
    int fd;
    xv_io_t *read_io;
    xv_io_t *write_io;
    xv_buffer_t *read_buffer;
    xv_buffer_t *write_buffer;
    xv_service_handle_t *handle;
    xv_io_thread_t *io_thread;
    xv_connection_status_t status;
    xv_atomic_t ref_count;
} xv_connection_t;

static xv_connection_t *xv_connection_init(const char *addr, int port, int fd,
                   xv_service_handle_t *handle, xv_io_cb_t read_cb, xv_io_cb_t write_cb)
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

    conn->status = XV_CONN_OPEN;
    xv_atomic_set(&conn->ref_count, 1);

    return conn;
}

static void xv_connection_stop(xv_loop_t *loop, xv_connection_t *conn)
{
    xv_io_stop(loop, conn->read_io);
    xv_io_stop(loop, conn->write_io);
}

static void xv_connection_destroy(xv_connection_t *conn)
{
    xv_io_destroy(conn->read_io);
    xv_io_destroy(conn->write_io);
    xv_buffer_destroy(conn->read_buffer);
    xv_buffer_destroy(conn->write_buffer);
    xv_free(conn);
}

const char *xv_connection_get_addr(xv_connection_t *conn)
{
    return conn->addr;
}

int xv_connection_get_port(xv_connection_t *conn)
{
    return conn->port;
}

int xv_connection_get_fd(xv_connection_t *conn)
{
    return conn->fd;
}

void xv_connection_incr_ref(xv_connection_t *conn)
{
    xv_atomic_incr(&conn->ref_count);
}

void xv_connection_decr_ref(xv_connection_t *conn)
{
    xv_atomic_decr(&conn->ref_count);
}

// ----------------------------------------------------------------------------------------
// xv_listener_t
// ----------------------------------------------------------------------------------------
struct xv_listener_t {
    char addr[XV_ADDR_LEN];        // bind addr
    int port;                      // listen port
    int listen_fd;
    xv_io_t *listen_io;            // listen_fd readable cb
    xv_service_handle_t handle;    // user cb handle
    xv_io_thread_t *io_thread;     // which io thread call `xv_io_start`

    xv_listener_t *next;
};

static xv_listener_t *xv_listener_init(const char *addr, int port, int fd, xv_service_handle_t handle, xv_io_cb_t new_conn_cb)
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
struct xv_message_t {
    xv_connection_t *conn;
    void *request;
    void *response;
};

static xv_message_t *xv_message_init(xv_connection_t *conn)
{
    xv_message_t *message = (xv_message_t *)xv_malloc(sizeof(xv_message_t));

    message->conn = conn;
    // incr conn ref_count
    xv_connection_incr_ref(conn);

    message->request = NULL;
    message->response = NULL;

    return message;
}

static void xv_message_destroy(xv_message_t *message, void (*packet_cleanup)(void *))
{
    if (packet_cleanup) {
        if (message->request) {
            packet_cleanup(message->request);
        }
        if (message->response) {
            packet_cleanup(message->response);
        }
    }
    // decr conn ref_count when message destroy
    xv_connection_decr_ref(message->conn);

    xv_free(message);
}

xv_connection_t *xv_message_get_connection(xv_message_t *message)
{
    return message->conn;
}

void *xv_message_get_request(xv_message_t *message)
{
    return message->request;
}

void *xv_message_get_response(xv_message_t *message)
{
    return message->response;
}

void xv_message_set_request(xv_message_t *message, void *request)
{
    message->request = request;
}

void xv_message_set_response(xv_message_t *message, void *response)
{
    message->response = response;
}

// ----------------------------------------------------------------------------------------
// xv_io_thread_t
// ----------------------------------------------------------------------------------------
struct xv_io_thread_t {
    int idx;
    pthread_t id;
    xv_loop_t *loop;
    xv_service_t *service;
    xv_async_t *async_add_conn;
    xv_concurrent_queue_t *conn_queue;
    xv_async_t *async_return_message;
    xv_concurrent_queue_t *message_queue;
};

static void io_thread_add_conn_cb(xv_loop_t *loop, xv_async_t *async)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)xv_async_get_userdata(async);

    // io thread add new connection
    while (xv_concurrent_queue_size(io_thread->conn_queue) > 0) {
        xv_connection_t *conn = xv_concurrent_queue_pop(io_thread->conn_queue);
        if (conn) {
            xv_log_debug("I'm follow IO Thread No.%d, add conn[%s:%d fd:%d] to my loop",
                    io_thread->idx, conn->addr, conn->port, conn->fd);

            conn->io_thread = io_thread;
            // chekck it
            if (loop != io_thread->loop) {
                xv_log_error("What? loop != io_thread->loop, check the code!");
            }
            xv_io_start(loop, conn->read_io);
        }
    }
}

static void process_message(xv_loop_t *loop, xv_message_t *message, xv_connection_t *conn, xv_service_handle_t *handle);

static void io_thread_return_message_cb(xv_loop_t *loop, xv_async_t *async)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)xv_async_get_userdata(async);

    // io thread process all message
    while (xv_concurrent_queue_size(io_thread->message_queue) > 0) {
        xv_message_t *message = xv_concurrent_queue_pop(io_thread->message_queue);
        if (message) {
            xv_connection_t *conn = xv_message_get_connection(message);
            xv_log_debug("I'm follow IO Thread No.%d, I got a return message: %p, conn[%s:%d fd:%d] to my loop",
                    io_thread->idx, message, conn->addr, conn->port, conn->fd);

            if (conn->status != XV_CONN_CLOSED) {
                process_message(loop, message, conn, conn->handle);
                xv_message_destroy(message, conn->handle->packet_cleanup);
            } else {
                xv_message_destroy(message, conn->handle->packet_cleanup);
                xv_connection_destroy(conn);
            }
        }
    }
}

static xv_io_thread_t *xv_io_thread_init(int i, xv_service_t *service)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)xv_malloc(sizeof(xv_io_thread_t));
    io_thread->idx = i;
    io_thread->loop = xv_loop_init(XV_DEFAULT_LOOP_SIZE);
    io_thread->service = service;

    // when new connection distribute to myself
    io_thread->conn_queue = xv_concurrent_queue_init();
    io_thread->async_add_conn = xv_async_init(io_thread_add_conn_cb);
    xv_async_set_userdata(io_thread->async_add_conn, io_thread);

    // when worker thread pool return the message
    io_thread->message_queue = xv_concurrent_queue_init();
    io_thread->async_return_message = xv_async_init(io_thread_return_message_cb);
    xv_async_set_userdata(io_thread->async_return_message, io_thread);

    return io_thread;
}

static void xv_io_thread_stop(xv_io_thread_t *io_thread)
{
    xv_async_stop(io_thread->loop, io_thread->async_add_conn);
    xv_async_stop(io_thread->loop, io_thread->async_return_message);

    // break loop
    xv_loop_break(io_thread->loop);
}

static void xv_io_thread_destroy(xv_io_thread_t *io_thread)
{
    xv_concurrent_queue_destroy(io_thread->conn_queue, (xv_queue_data_destroy_cb_t)xv_connection_destroy);
    xv_async_destroy(io_thread->async_add_conn);
    xv_concurrent_queue_destroy(io_thread->message_queue, (xv_queue_data_destroy_cb_t)xv_message_destroy);
    xv_async_destroy(io_thread->async_return_message);
    xv_loop_destroy(io_thread->loop);
    xv_free(io_thread);
}

// ----------------------------------------------------------------------------------------
// xv_service_t
// ----------------------------------------------------------------------------------------
struct xv_service_t {
    xv_service_config_t config;
    xv_io_thread_t **io_threads;
    xv_thread_pool_t *worker_threads;
    xv_listener_t *listeners;
    int conn_setsize;
    xv_connection_t **connections;
    xv_atomic_t conn_count;
    int start;
};

// not thread safe, but just leader io call this function
static void xv_service_add_connection(xv_service_t *service, xv_connection_t *conn);
static int xv_service_del_connection(xv_service_t *service, xv_connection_t *conn);

static void xv_connection_close(xv_connection_t *conn)
{
    if (conn->status != XV_CONN_CLOSED) {
        conn->status = XV_CONN_CLOSED;
        // call user on_disconnect
        if (conn->handle->on_disconnect) {
            conn->handle->on_disconnect(conn);
        }
        xv_connection_stop(conn->io_thread->loop, conn);
    }
    // some xv_message_t ref to this xv_connection_t, just return
    if (xv_atomic_get(&conn->ref_count) > 1) {
        xv_log_debug("connection ref_count is %d, cannot drop it, return", xv_atomic_get(&conn->ref_count));
        return;
    }
    xv_service_del_connection(conn->io_thread->service, conn);

    // close last but before destroy
    xv_close(conn->fd);

    xv_connection_destroy(conn);
}

int xv_service_send_message(xv_connection_t *conn, void *package)
{
    if (!conn || conn->status == XV_CONN_CLOSED) {
        xv_log_error("conn is closed, cannot send message!");
        return XV_ERR;
    }
    xv_message_t *message = xv_message_init(conn);
    xv_message_set_response(message, package);  // set response, ignore request

    // push message to io thread
    xv_io_thread_t *io_thread = conn->io_thread;
    xv_concurrent_queue_push(io_thread->message_queue, message);
    xv_async_send(io_thread->async_return_message);

    return XV_OK;
}

typedef struct xv_service_pool_task_t {
    int (*cb)(xv_message_t *);
    xv_message_t *message;
} xv_service_pool_task_t;

static void thread_pool_task_cb(void *args)
{
    xv_service_pool_task_t *task = (xv_service_pool_task_t *)args;
    if (task->cb && task->message) {
        task->cb(task->message);
    }

    // push message to io thread
    xv_io_thread_t *io_thread = xv_message_get_connection(task->message)->io_thread;
    xv_concurrent_queue_push(io_thread->message_queue, task->message);
    xv_async_send(io_thread->async_return_message);

    xv_free(task);
}

static void process_message(xv_loop_t *loop, xv_message_t *message, xv_connection_t *conn, xv_service_handle_t *handle)
{
    void *response = xv_message_get_response(message);
    if (!response || !handle->encode) {
        xv_log_debug("response: %p, handle->encode: %p, cannot process message, return", response, handle->encode);
        return;
    }
    handle->encode(conn->write_buffer, response);
    int want_write_size = xv_buffer_readable_size(conn->write_buffer);
    if (want_write_size == 0) {
        return;
    }
    int nwritten = write(conn->fd, xv_buffer_read_begin(conn->write_buffer), want_write_size);
    if (nwritten == 0 || (nwritten == -1 && errno != EAGAIN && errno != EINTR)) {
        xv_log_errno_error("xv_write return failed, close connection now, error");
        xv_connection_close(conn);
    } else {
        if (nwritten > 0) {
            // incr buffer index
            xv_buffer_incr_read_index(conn->write_buffer, nwritten);
        }
        // check write size
        if (nwritten < want_write_size && conn->status == XV_CONN_OPEN) {
            // unhappy, kernel socket buffer is full, start write event
            xv_io_start(loop, conn->write_io);
        }
    }
}

static void process_read_buffer(xv_loop_t *loop, xv_connection_t *conn, xv_service_handle_t *handle)
{
    // do user decode
    if (!handle->decode || !handle->process) {
        xv_log_debug("handle->decode: %p, handle->process: %p, read buffer drop and return", handle->decode, handle->process);
        // clear buffer, drop data and return
        xv_buffer_clear(conn->read_buffer);
        return;
    }
    void *request = NULL;
    int ret = handle->decode(conn->read_buffer, &request);
    if (ret == XV_OK) {
        //  do user process
        xv_message_t *message = xv_message_init(conn);
        xv_message_set_request(message, request);

        xv_thread_pool_t *worker_threads = conn->io_thread->service->worker_threads;
        if (!worker_threads) {
            // do process in self io thread
            handle->process(message);
            process_message(loop, message, conn, handle);
            xv_message_destroy(message, handle->packet_cleanup);
        } else {
            xv_service_pool_task_t *task = (xv_service_pool_task_t *)xv_malloc(sizeof(xv_service_pool_task_t));
            task->cb = handle->process;
            task->message = message;
            xv_log_debug("we have worker threa pool, now push task");
            // move message to worker thread pool
            xv_thread_pool_push_task(worker_threads, thread_pool_task_cb, task, ((uint64_t)task) > 16);
        }
    } else if (ret == XV_ERR) {
        // decode failed! close it
        xv_connection_close(conn);
    }

    // if XV_AGAIN, do nothing...
}

static void on_connection_read(xv_loop_t *loop, xv_io_t *io)
{
    int fd = xv_io_get_fd(io);
    xv_connection_t *conn = (xv_connection_t *)xv_io_get_userdata(io);
    xv_service_handle_t *handle = conn->handle;

    if (conn->status == XV_CONN_CLOSED) {
        return;
    }

    // max read `XV_DEFAULT_READ_SIZE` bytes
    xv_buffer_ensure_writeable_size(conn->read_buffer, XV_DEFAULT_READ_SIZE);

    int nread = read(fd, xv_buffer_write_begin(conn->read_buffer), XV_DEFAULT_READ_SIZE);
    if (nread <= 0) {
        if (nread == -1 && (errno == EAGAIN || errno == EINTR)) {
            return;
        }
        xv_log_errno_error("xv_read return failed, close connection now, error");

        // will close it
        xv_connection_close(conn);
    } else {
        xv_log_debug("read from fd: %d, nread: %d", conn->fd, nread);

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
        int nwritten = write(conn->fd, xv_buffer_read_begin(conn->write_buffer), buffer_size);
        if (nwritten == 0 || (nwritten == -1 && errno != EAGAIN && errno != EINTR)) {
            xv_log_errno_error("xv_write return failed, close connection now, error");

            xv_connection_close(conn);
        } else {
            if (nwritten > 0) {
                // incr buffer index
                xv_buffer_incr_read_index(conn->write_buffer, nwritten);
            }
            if (nwritten == buffer_size) {
                // happy, write all data success, stop write event
                xv_io_stop(loop, conn->write_io);
            }
        }
    } else {
        xv_io_stop(loop, conn->write_io);
    }
}

// just leader io thread call this function
static void on_new_connection(xv_loop_t *loop, xv_io_t *io)
{
    int listen_fd = xv_io_get_fd(io);

    char addr[XV_ADDR_LEN];
    int port;
    int client_fd = xv_tcp_accept(listen_fd, addr, sizeof(addr), &port);
    if (client_fd > 0) {
        xv_log_debug("xv_tcp_accept new connection: %s:%d", addr, port);

        int ret = xv_nonblock(client_fd);
        if (ret != XV_OK) {
            xv_close(client_fd);
            return;
        }
        xv_listener_t *listener = (xv_listener_t *)xv_io_get_userdata(io);
        xv_service_t *service = listener->io_thread->service;
        if (service->config.tcp_nodealy) {
            ret = xv_tcp_nodelay(client_fd);
            if (ret != XV_OK) {
                xv_close(client_fd);
                return;
            }
        }
        xv_service_handle_t *handle = &listener->handle;
        xv_connection_t *conn = xv_connection_init(addr, port, client_fd, handle, on_connection_read, on_connection_write);

        // add conn to service
        xv_service_add_connection(listener->io_thread->service, conn);

        // user on_conn callback
        if (handle->on_connect) {
            handle->on_connect(conn);
        }

        int io_thread_count = service->config.io_thread_count;
        // add conn to myself conn list or send conn to other io thread
        if (io_thread_count == 1) {
            conn->io_thread = listener->io_thread;
            // start socket READ event to myself loop
            xv_io_start(loop, conn->read_io);
        } else {
            // send this conn to other io thread
            int index = conn->fd % (io_thread_count - 1) + 1;
            xv_concurrent_queue_push(service->io_threads[index]->conn_queue, conn);
            xv_async_send(service->io_threads[index]->async_add_conn);
        }
    }
}

static void *io_thread_entry(void *args)
{
    xv_io_thread_t *io_thread = (xv_io_thread_t *)args;
    xv_service_t *service = io_thread->service;

    // start all async
    xv_async_start(io_thread->loop, io_thread->async_add_conn);
    xv_async_start(io_thread->loop, io_thread->async_return_message);

    if (io_thread->idx == 0) {
        xv_log_debug("I'am leader IO Thread, add all listen fd event");

        xv_listener_t *listener = service->listeners;
        while (listener) {
            xv_log_debug("leader IO Thread add listener, addr: %s:%d", listener->addr, listener->port);

            listener->io_thread = io_thread;
            xv_io_start(io_thread->loop, listener->listen_io);
            listener = listener->next;
        }
    } else {
        xv_log_debug("I'am follower IO Thread No.%d, wait Leader send xv_connection_t", io_thread->idx);
    }

    // loop run until service stop
    xv_loop_run_timeout(io_thread->loop, 10);  // 100 times per second

    if (io_thread->idx == 0) {
        xv_log_debug("I'am leader IO Thread, del all listen fd evebt");

        xv_listener_t *listener = service->listeners;
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

xv_service_t *xv_service_init(xv_service_config_t config)
{
    if (config.io_thread_count <= 0 || config.worker_thread_count < 0) {
        xv_log_error("config.io_thread_count must > 0, config.worker_thread_count must >= 0");
        return NULL;
    }
    xv_service_t *service = (xv_service_t *)xv_malloc(sizeof(xv_service_t));
    service->io_threads = (xv_io_thread_t **)xv_malloc(sizeof(xv_io_thread_t *) * config.io_thread_count);
    for (int i = 0; i < config.io_thread_count; ++i) {
        service->io_threads[i] = xv_io_thread_init(i, service);
    }
    if (config.worker_thread_count > 0) {
        service->worker_threads = xv_thread_pool_init(config.worker_thread_count);
    } else {
        service->worker_threads = NULL;
    }
    service->config = config;
    service->listeners = NULL;

    // init connections set
    int array_size = sizeof(xv_connection_t *) * XV_DEFAULT_LOOP_SIZE;
    service->connections = (xv_connection_t **)xv_malloc(array_size);
    memset(service->connections, 0, array_size);

    service->conn_setsize = XV_DEFAULT_LOOP_SIZE;
    xv_atomic_set(&service->conn_count, 0);

    service->start = 0;

    return service;
}

int xv_service_add_listen(xv_service_t *service, const char *addr, int port, xv_service_handle_t handle)
{
    int listen_fd = xv_tcp_listen(addr, port, 1024);
    if (listen_fd < 0) {
        xv_log_error("listen on %s:%d failed!");
        return XV_ERR;
    }
    int ret = xv_nonblock(listen_fd);
    if (ret != XV_OK) {
        xv_close(listen_fd);
        return XV_ERR;
    }

    xv_listener_t *listener = xv_listener_init(addr, port, listen_fd, handle, on_new_connection);

    // link to service->listeners's head
    listener->next = service->listeners;
    service->listeners = listener;

    return XV_OK;
}

// not thread safe, but just leader io call this function
static void xv_service_add_connection(xv_service_t *service, xv_connection_t *conn)
{
    // resize conn setsize
    if (conn->fd > service->conn_setsize) {
        xv_log_debug("conn->fd: %d, service->conn_setsiz: %d, resize the service->conn_setsize to %d",
                conn->fd, service->conn_setsize, service->conn_setsize * 2);

        service->conn_setsize *= 2;
        int array_size = sizeof(xv_connection_t *) * service->conn_setsize;
        service->connections = (xv_connection_t **)xv_realloc(service->connections, array_size);
    }
    xv_log_debug("add conn[%s:%d, fd: %d] to service", conn->addr, conn->port, conn->fd);

    service->connections[conn->fd] = conn;
    xv_memory_barriers();

    xv_atomic_incr(&service->conn_count);
}

static int xv_service_del_connection(xv_service_t *service, xv_connection_t *conn)
{
    if (conn->fd < 0 || conn->fd >= service->conn_setsize) {
        xv_log_error("conn->fd: %d, service->conn_setsiz: %d, del failed, check the code", conn->fd, service->conn_setsize);
        return XV_ERR;
    }
    xv_log_debug("del conn[%s:%d, fd: %d] from service", conn->addr, conn->port, conn->fd);

    service->connections[conn->fd] = NULL;
    xv_memory_barriers();

    xv_atomic_decr(&service->conn_count);

    return XV_OK;
}

int xv_service_start(xv_service_t *service)
{
    xv_log_debug("xv_service starting...");

    if (service->start) {
        xv_log_error("service already started!");
        return XV_ERR;
    }
    service->start = 1;
    xv_memory_barriers();

    if (service->worker_threads) {
        xv_thread_pool_start(service->worker_threads);
    }
    for (int i = 0; i < service->config.io_thread_count; ++i) {
        int ret = pthread_create(&service->io_threads[i]->id, NULL, io_thread_entry, service->io_threads[i]);
        if (ret != 0) {
            xv_log_errno_error("pthread_create io thread failed!");
            return XV_ERR;
        }
    }

    return XV_OK;
}

int xv_service_run(xv_service_t *service)
{
    xv_log_debug("xv_service running...");

    if (!service->start) {
        xv_log_error("service is not started!");
        return XV_ERR;
    }
    for (int i = 0; i < service->config.io_thread_count; ++i) {
        int ret = pthread_join(service->io_threads[i]->id, NULL);
        if (ret != 0) {
            xv_log_errno_error("pthread_join io thread failed!");
            return XV_ERR;
        }
    }

    return XV_OK;
}

int xv_service_stop(xv_service_t *service)
{
    xv_log_debug("xv_service will stop...");

    if (service->start == 0) {
        return XV_ERR;
    }
    service->start = 0;
    xv_memory_barriers();

    // stop all listeners
    xv_log_debug("stop all listeners...");
    xv_listener_t *listener = service->listeners;
    while (listener) {
        xv_listener_stop(listener->io_thread->loop, listener);
        listener = listener->next;
    }

    // stop all connection
    xv_log_debug("stop all listeners...");
    for (int i = 0; i < service->conn_setsize; ++i) {
        if (service->connections[i]) {
            xv_connection_stop(service->connections[i]->io_thread->loop, service->connections[i]);
        }
    }

    // stop all io thread
    xv_log_debug("stop all io thread...");
    for (int i = 0; i < service->config.io_thread_count; ++i) {
        xv_io_thread_stop(service->io_threads[i]);
    }

    // stop worker thread pool
    xv_log_debug("stop worker thread pool...");
    if (service->worker_threads) {
        xv_thread_pool_stop(service->worker_threads);
    }

    return XV_OK;
}

void xv_service_destroy(xv_service_t *service)
{
    xv_service_stop(service);

    xv_log_debug("xv_service will destroy");

    // destroy all listeners
    xv_log_debug("destroy all listeners");
    xv_listener_t *listener = service->listeners;
    while (listener) {
        xv_listener_t *tmp = listener->next;
        xv_listener_destroy(listener);
        listener = tmp;
    }

    // destory all connection
    xv_log_debug("destory all connection...");
    for (int i = 0; i < service->conn_setsize; ++i) {
        if (service->connections[i]) {
            xv_connection_destroy(service->connections[i]);
        }
    }
    xv_free(service->connections);

    // destroy all io thread
    xv_log_debug("destroy all io thread...");
    for (int i = 0; i < service->config.io_thread_count; ++i) {
        xv_io_thread_destroy(service->io_threads[i]);
    }
    xv_free(service->io_threads);

    // destroy worker thread pool
    xv_log_debug("destroy all worker thread pool...");
    if (service->worker_threads) {
        xv_thread_pool_destroy(service->worker_threads);
    }

    xv_free(service);
}

