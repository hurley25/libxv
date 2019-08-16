/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_service_test.c 08/13/2019 $
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

#define SEND_STR "hello xv!"
#define TEST_PORT 12345
#define TEST_THREAD_COUNT 4
#define TEST_COUNT 50

void connect_once()
{
    const char *str = SEND_STR;
    int fd = xv_tcp_connect("127.0.0.1", TEST_PORT);
    CHECK(fd > 0, "xv_tcp_connect: ");

    int ret = xv_tcp_nodelay(fd);
    CHECK(ret == XV_OK, "xv_tcp_nodelay(fd) failed!");

    const int len = strlen(str);
    for (int i = 0; i < len; ++i) {
            int ret = xv_block_write(fd, str + i, 1);
            usleep(1000);
            CHECK(ret == 1, "write: ");
        }

    char buf[len + 1];
    ret = xv_block_read(fd, buf, len);
    CHECK(ret > 0, "read: ");
    CHECK(ret == len, "read size != write size");

    buf[len] = '\0';
    fprintf(stderr, "%s\n", buf);

    CHECK(memcmp(str, buf, ret) == 0, "read data != write data");

    xv_close(fd);
}

void *client_fun(void *args)
{
    int idx = *(int *)args;
    xv_free(args);

    for (int i = 0; i < TEST_COUNT; ++i) {
        connect_once();
    }

    if (idx == 0) {
        usleep(100000);
        kill(getpid(), SIGINT);
    }

    return NULL;
}

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
    packet_t *response = (packet_t *)xv_malloc(sizeof(int) + request->len);
    memcpy(response->buf, request->buf, request->len);
    response->len = request->len;

    xv_message_set_response(message, response);

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
    fprintf(stderr, "new connection: %s:%d\n",
            xv_connection_get_addr(conn), xv_connection_get_port(conn));
}

void on_disconnect(xv_connection_t *conn)
{
    fprintf(stderr, "close connection: %s:%d\n",
            xv_connection_get_addr(conn), xv_connection_get_port(conn));
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

    pthread_t ids[TEST_THREAD_COUNT];
    for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
        int *pi = (int *)xv_malloc(sizeof(int));
        *pi = i;
        ret = pthread_create(&ids[i], NULL, client_fun, pi);
        CHECK(ret == 0, "pthread_create: ");
    }

    ret = xv_service_run(service);
    ASSERT(ret == XV_OK);

    for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
        ret = pthread_join(ids[i], NULL);
        CHECK(ret == 0, "pthread_create: ");
    }

    xv_service_destroy(service);

    return EXIT_SUCCESS;
}

