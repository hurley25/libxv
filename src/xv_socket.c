/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_socket.c 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_socket.h"

#define _GNU_SOURCE
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include "xv_log.h"

static int xv_tcp_reuse_addr(int fd) {
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        xv_log_errno_error("setsockopt failed");
        return XV_ERR;
    }
    return XV_OK;
}

static int xv_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        xv_log_errno_error("socket failed");
        return XV_ERR;
    }
    if (xv_tcp_reuse_addr(sock) == XV_ERR) {
        xv_close(sock);
        return XV_ERR;
    }
    return sock;
}

static int xv_tcp_generic_connect(const char *addr, int port, int nonblock)
{
    int sock = xv_socket();
    if (sock == XV_ERR) {
        return XV_ERR;
    }
    if (nonblock) {
        if (xv_nonblock(sock) == XV_ERR) {
            xv_close(sock);
            return XV_ERR;
        }
    }
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &sa.sin_addr) < 0) {
        xv_log_errno_error("inet_pton failed");
        return XV_ERR;
    }

    int ret = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if (ret < 0) {
        xv_log_errno_error("connect failed");
        return XV_ERR;
    }

    xv_log_debug("connect to %s:%d", addr, port);
    
    return sock;
}

int xv_tcp_connect(const char *addr, int port)
{
    return xv_tcp_generic_connect(addr, port, 0);
}

int xv_tcp_nonblock_connect(const char *addr, int port)
{
    return xv_tcp_generic_connect(addr, port, 1);
}

int xv_tcp_listen(const char *addr, int port, int backlog)
{
    int sock = xv_socket();
    if (sock == XV_ERR) {
        return XV_ERR;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &sa.sin_addr) < 0) {
        xv_log_errno_error("inet_pton failed");
        return XV_ERR;
    }

    if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        xv_log_errno_error("bind failed");
        return XV_ERR;
    }

    if (listen(sock, backlog) < 0) {
        xv_log_errno_error("listen failed");
        return XV_ERR;
    }

    xv_log_debug("listen on %s:%d, backlog is %d", addr, port, backlog);

    return sock;
}

int xv_tcp_accept(int fd, char *client_ip, int client_ip_len, int *port)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    int cfd = -1;

    while (1) {
        cfd = accept(fd, (struct sockaddr *)&sa, &len);
        if (cfd < 0) {
            if (errno == EINTR) {
                continue;
            }
            xv_log_errno_error("accept failed");
            return XV_ERR;
        }
        break;
    }

    if (xv_curr_log_level == XV_LOG_DEBUG) {
        char conn_ip[16] = {0};
        inet_ntop(AF_INET, &(sa.sin_addr), conn_ip, sizeof(conn_ip));
        xv_log_debug("new connection, ip: %s, port: %d", conn_ip, ntohs(sa.sin_port));
    }

    if (client_ip) {
        inet_ntop(AF_INET, &(sa.sin_addr), client_ip, client_ip_len);
    }
    if (port) {
        *port = ntohs(sa.sin_port);
    }

    return cfd;
}

int xv_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        xv_log_errno_error("fcntl failed");
        return XV_ERR;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        xv_log_errno_error("fcntl failed");
        return XV_ERR;
    }

    return XV_OK;
}

int xv_tcp_nodelay(int fd)
{
    int val = 1;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        xv_log_errno_error("setsockopt failed");
        return XV_ERR;
    }

    return XV_OK;
}

int xv_block_read(int fd, char *buf, int count)
{
    int nread = 0;
    while (nread != count) {
        int ret = read(fd, buf, count - nread);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ret == 0) {
            return nread;
        }
        nread += ret;
        buf += ret;
    }

    return nread;
}

int xv_block_write(int fd, const char *buf, int count)
{
    int nwritten = 0;
    while (nwritten != count) {
        int ret = write(fd, buf, count - nwritten);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ret == 0) {
            return nwritten;;
        }
        nwritten += ret;
        buf += ret;
    }

    return nwritten;
}

int xv_close(int fd)
{
    return close(fd);
}

