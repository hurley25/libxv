/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_server.h 08/12/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_SERVER_H_
#define XV_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "xv.h"
#include "xv_atomic.h"
#include "xv_buffer.h"

#define XV_ADDR_LEN 32

typedef struct xv_server_t xv_server_t;
typedef struct xv_io_thread_t xv_io_thread_t;
typedef struct xv_listener_t xv_listener_t;
typedef struct xv_connection_t xv_connection_t;
typedef struct xv_message_t xv_message_t;

// server init config
typedef struct xv_server_config_t {
    int io_thread_count;
    int worker_thread_count;
    int tcp_nodealy;
    int io_affinity_enable;  // now support yet
} xv_server_config_t;

// handle for listen port
typedef struct xv_server_handle_t {
    int (*decode)(xv_buffer_t *, void **);     // user packet decode, origin data read from `xv_buffer_t`
    int (*encode)(xv_buffer_t *, void *);      // user packet encode, write data to `xv_buffer_t`
    int (*process)(xv_message_t *);            // process request, call `xv_message_get_request()` &  `xv_message_set_response()`
    void (*packet_cleanup)(void *);            // cleanup user's packet
    void (*on_send_failed)(void *);            // when send to connection failed, such as fd closed
    void (*on_connect)(xv_connection_t *);     // when `accept` a new connection
    void (*on_disconnect)(xv_connection_t *);  // when connection will disconnect
} xv_server_handle_t;

// ----------------------------------------------------------------------------------------
// xv_server_t
// ----------------------------------------------------------------------------------------
xv_server_t *xv_server_init(xv_server_config_t config);
int xv_server_add_listen(xv_server_t *server, const char *addr, int port, xv_server_handle_t handle);
int xv_server_start(xv_server_t *server);
int xv_server_run(xv_server_t *server);
int xv_server_stop(xv_server_t *server);
void xv_server_destroy(xv_server_t *server);

// ----------------------------------------------------------------------------------------
// xv_connection_t
// ----------------------------------------------------------------------------------------
const char *xv_connection_get_addr(xv_connection_t *conn);
int xv_connection_get_port(xv_connection_t *conn);
int xv_connection_get_fd(xv_connection_t *conn);
void xv_connection_incr_ref(xv_connection_t *conn);
void xv_connection_decr_ref(xv_connection_t *conn);
int xv_server_send_message(xv_connection_t *conn, void *package);

// ----------------------------------------------------------------------------------------
// xv_message_t
// ----------------------------------------------------------------------------------------
xv_connection_t *xv_message_get_connection(xv_message_t *message);
void *xv_message_get_request(xv_message_t *message);
void *xv_message_get_response(xv_message_t *message);
void xv_message_set_request(xv_message_t *message, void *request);
void xv_message_set_response(xv_message_t *message, void *response);

#ifdef __cplusplus
}
#endif

#endif // XV_SERVER_H_

