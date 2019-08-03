/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_socket.h 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_SOCKETS_H_
#define XV_SOCKETS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xv_define.h"

int xv_tcp_connect(const char *addr, int port);
int xv_tcp_nonblock_connect(const char *addr, int port);

int xv_tcp_listen(const char *addr, int port, int backlog);
int xv_tcp_accept(int fd, char *client_ip, int client_ip_len, int *port);

int xv_nonblock(int fd);
int xv_tcp_nodelay(int fd);

int xv_read(int fd, char *buf, int count);
int xv_write(int fd, const char *buf, int count);

int xv_close(int fd);

#ifdef __cplusplus
}
#endif

#endif // XV_SOCKETS_H_

