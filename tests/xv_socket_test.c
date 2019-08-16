/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_socket_tests.c 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <unistd.h>

#include "xv_test.h"
#include "xv_socket.h"

#define TEST_PORT 8086
#define SEND_STR "Hello libxv!"

void *client_fun(void *args)
{
    const char *str = (const char *)args;
    int fd = xv_tcp_connect("127.0.0.1", TEST_PORT);
    CHECK(fd > 0, "xv_tcp_connect: ");

    const int len = strlen(str);
    for (int i = 0; i < len; ++i) {
        int ret = xv_block_write(fd, str + i, 1);
        CHECK(ret == 1, "write: ");
    }

    char buf[len + 1];
    int ret = xv_block_read(fd, buf, len);
    CHECK(ret > 0, "read: ");
    CHECK(ret == len, "read size != write size");

    buf[len] = '\0';
    fprintf(stderr, "%s\n", buf);

    CHECK(memcmp(str, buf, ret) == 0, "read data != write data");

    xv_close(fd);

    return NULL;
}

int main(int argc, char *argv[])
{
    int server_fd = xv_tcp_listen("0.0.0.0", TEST_PORT, 1024);
    CHECK(server_fd > 0, "xv_tcp_listen: ");

    pthread_t id;
    int ret = pthread_create(&id, NULL, client_fun, SEND_STR);
    CHECK(ret == 0, "pthread_create: ");
   
    char client_ip[16] = {0};
    int client_port = 0;
    int client_fd = xv_tcp_accept(server_fd, client_ip, sizeof(client_ip), &client_port);
    CHECK(client_fd > 0, "xv_tcp_accept: ");

    const int len = strlen(SEND_STR);
    char buf[len + 1];
    ret = xv_block_read(client_fd, buf, len);
    CHECK(ret > 0, "read: ");
    CHECK(ret == len, "read size != write size");

    ret = xv_block_write(client_fd, buf, len);
    CHECK(ret > 0, "write: ");
    CHECK(ret == len, "read size != write size");

    xv_close(client_port);
    xv_close(server_fd);

    ret = pthread_join(id, NULL);
    CHECK(ret == 0, "pthread_join: ");

    return EXIT_SUCCESS;
}

