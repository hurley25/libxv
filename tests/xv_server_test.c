/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_server_test.c 08/13/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include "xv_test.h"
#include "xv_server.h"

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
    ASSERT(message->request);
    packet_t *request = (packet_t *)message->request;
    packet_t *response = (packet_t *)xv_malloc(sizeof(int) + request->len);
    memcpy(response->buf, request->buf, request->len);
    response->len = request->len;
    message->response = response;

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
    fprintf(stderr, "new connection: %s:%d\n", conn->addr, conn->port);
}

void on_disconnect(xv_connection_t *conn)
{
    fprintf(stderr, "close connection: %s:%d\n", conn->addr, conn->port);
}

xv_server_t *server = NULL;

void handle_sigint(int sig)
{
    if (sig == SIGINT) {
        fprintf(stderr, "recv sigint, exit now\n");
        if (server) {
            xv_server_stop(server);
        }
    }
}

int main(int argc, char *argv[])
{
    //xv_set_log_level(XV_LOG_DEBUG);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);

    xv_server_handle_t handle;
    bzero(&handle, sizeof(handle));
    handle.on_connect = on_connect;
    handle.on_disconnect = on_disconnect;
    handle.decode = decode;
    handle.process = process;
    handle.encode = encode;
    handle.packet_cleanup = packet_cleanup;

    xv_server_config_t config;
    config.io_thread_count = 1;
    config.worker_thread_count = 0;
    config.tcp_nodealy = 1;
    config.affinity_enable = 0;

    server = xv_server_init(config);
    ASSERT(server);

    int ret = xv_server_add_listen(server, "0.0.0.0", 12345, handle);
    ASSERT(ret == XV_OK);

    ret = xv_server_start(server);
    ASSERT(ret == XV_OK);

    ret = xv_server_run(server);
    ASSERT(ret == XV_OK);

    xv_server_destroy(server);

    return EXIT_SUCCESS;
}

