/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_loop_socket_test.c 08/04/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "xv_test.h"
#include "xv_socket.h"

#define BIND_ADDR "0.0.0.0"
#define TEST_PORT 8086
#define SEND_STR "Hello libxv!"

// client use blocking socket
void *client_fun(void *args)
{
    const char *str = (const char *)args;
    int fd = xv_tcp_connect("127.0.0.1", TEST_PORT);
    CHECK(fd > 0, "xv_tcp_connect: ");

    const int len = strlen(str);
    int ret = xv_write(fd, str, len);
    CHECK(ret == len, "write: ");

    char buf[len + 1];
    ret = xv_read(fd, buf, len);
    CHECK(ret > 0, "read: ");
    CHECK(ret == len, "read size != write size");

    buf[len] = '\0';
    fprintf(stderr, "%s\n", buf);

    CHECK(memcmp(str, buf, ret) == 0, "read data != write data");

    xv_close(fd);

    return NULL;
}

void on_server_read(xv_io_t *io)
{
    const int len = strlen(SEND_STR);
    char buf[len + 1];

    int fd = xv_io_get_fd(io);

    // maybe cannot read full data
    int ret = xv_read(fd, buf, len);
    CHECK(ret > 0, "read: ");
    CHECK(ret == len, "read size != write size");

    // write back
    ret = xv_write(fd, buf, len);
    CHECK(ret > 0, "write: ");
    CHECK(ret == len, "read size != write size");

    // break here
    xv_loop_break(xv_io_get_loop(io));

    // close before destroy
    xv_close(fd);

    // auto stop io
    xv_io_destroy(io);
}

void on_new_connection(xv_io_t *io)
{
    char client_ip[16] = {0};
    int client_port = 0;

    int server_fd = xv_io_get_fd(io);
    xv_loop_t *loop = xv_io_get_loop(io);

    int client_fd = xv_tcp_accept(server_fd, client_ip, sizeof(client_ip), &client_port);

    if (client_fd > 0) {
        xv_io_t *io_client = xv_io_init(client_fd, XV_READ, on_server_read);
        xv_io_start(loop, io_client);
    }
}

int main(int argc, char *argv[])
{
    xv_set_log_level(XV_LOG_DEBUG);

    xv_loop_t *loop = xv_loop_init(1024);

    int server_fd = xv_tcp_listen(BIND_ADDR, TEST_PORT, 1024);
    ASSERT(server_fd > 0);
    int ret = xv_nonblock(server_fd);
    ASSERT(ret == XV_OK);

    pthread_t id;
    ret = pthread_create(&id, NULL, client_fun, SEND_STR);
    CHECK(ret == 0, "pthread_create: ");

    xv_io_t *io_server = xv_io_init(server_fd, XV_READ, on_new_connection);
    xv_io_start(loop, io_server);

    // blockiong here
    xv_loop_run(loop);

    ret = pthread_join(id, NULL);
    CHECK(ret == 0, "pthread_join: ");

    // io auto stop
    xv_io_destroy(io_server);

    xv_loop_destroy(loop);

    return EXIT_SUCCESS;
}

