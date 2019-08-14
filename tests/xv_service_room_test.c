/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_service_room_test.c 08/14/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "xv_test.h"
#include "xv_service.h"
#include "xv_socket.h"

#define TEST_PORT 12345
#define TEST_COUNT 10

#define MAX_CONN 100

// just for demo test
xv_connection_t *conn_array[MAX_CONN];
pthread_mutex_t mutex;

typedef struct packet_t {
    int len;
    char buf[0];
} packet_t;

int decode(xv_buffer_t *buffer, void **request)
{
    int size = xv_buffer_readable_size(buffer);
    packet_t *req = (packet_t *)xv_malloc(sizeof(int) + size);
    int readn = xv_buffer_read_data(buffer, req->buf, size);
    req->len = readn;
    *request = req;

    ASSERT(readn == size);
    ASSERT(xv_buffer_readable_size(buffer) == 0);

    return XV_OK;
}

int process(xv_message_t *message)
{
    packet_t *request = (packet_t *)xv_message_get_request(message);

    for (int i = 0; i < MAX_CONN; ++i) {
        pthread_mutex_lock(&mutex);
        if (conn_array[i]) {
            packet_t *response = (packet_t *)xv_malloc(sizeof(int) + request->len);
            memcpy(response->buf, request->buf, request->len);
            response->len = request->len;

            xv_service_send_message(conn_array[i], response);
        }
        pthread_mutex_unlock(&mutex);
    }

    return XV_OK;
}

int encode(xv_buffer_t *buffer, void *reponse)
{
    packet_t *resp = (packet_t *)reponse;
    xv_buffer_write_data(buffer, resp->buf, resp->len);

    return XV_OK;
}

void packet_cleanup(void *packet)
{
    xv_free(packet);
}

void on_connect(xv_connection_t *conn)
{
    fprintf(stderr, "new connection: %s:%d\n", xv_connection_get_addr(conn), xv_connection_get_port(conn));

    int fd = xv_connection_get_fd(conn);
    if (fd > MAX_CONN) {
        fprintf(stderr, "MAX_CONN to small, fd is %d, exit", fd);
        exit(-1);
    }

    pthread_mutex_lock(&mutex);

    xv_connection_incr_ref(conn);
    conn_array[fd] = conn;

    pthread_mutex_unlock(&mutex);
}

void on_disconnect(xv_connection_t *conn)
{
    fprintf(stderr, "close connection: %s:%d\n", xv_connection_get_addr(conn), xv_connection_get_port(conn));

    int fd = xv_connection_get_fd(conn);
    pthread_mutex_lock(&mutex);

    conn_array[fd] = NULL;
    xv_connection_decr_ref(conn);

    pthread_mutex_unlock(&mutex);
}

xv_service_t *service = NULL;

void handle_sigint(int sig)
{
    if (sig == SIGINT) {
        fprintf(stderr, "recv sigint, exit now\n");
        if (service) {
            xv_service_stop(service);
        }
    }
}

int main(int argc, char *argv[])
{
    // xv_set_log_level(XV_LOG_DEBUG);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);

    pthread_mutex_init(&mutex, NULL);

    xv_service_handle_t handle;
    bzero(&handle, sizeof(handle));
    handle.on_connect = on_connect;
    handle.on_disconnect = on_disconnect;
    handle.decode = decode;
    handle.process = process;
    handle.encode = encode;
    handle.packet_cleanup = packet_cleanup;

    xv_service_config_t config;
    config.io_thread_count = 4;
    config.worker_thread_count = 4;
    config.tcp_nodealy = 1;

    service = xv_service_init(config);
    ASSERT(service);

    int ret = xv_service_add_listen(service, "0.0.0.0", TEST_PORT, handle);
    ASSERT(ret == XV_OK);

    ret = xv_service_start(service);
    ASSERT(ret == XV_OK);

    ret = xv_service_run(service);
    ASSERT(ret == XV_OK);

    xv_service_destroy(service);

    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}

