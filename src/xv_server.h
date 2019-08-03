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

typedef struct xv_server_t xv_server_t;
typedef struct xv_connection_t xv_connection_t;
typedef struct xv_buffer_t xv_buffer_t;
typedef struct xv_message_t xv_message_t;

typedef struct xv_server_config_t {
    int io_thread_count;
    int io_worker_count;
    int tcp_nodealy;
    int affinity_enable;
} xv_server_config_t;

typedef struct xv_server_handle_t {
    int (*decode)(xv_buffer *, void *);        // user packet decode, origin data read from `xv_buffer_t`, return `XV_OK`/`XV_ERR`/`XV_AGAIN`
    int (*encode)(xv_buffer *, void *);        // user packet encode, write data to `xv_buffer_t`, return XV_ERR will close this connection
    int (*process)(xv_message_t *);            // process request, call `xv_message_get_request()` &  `xv_message_set_response()`
    int (*on_connect)(xv_connection_t *);      // when `accept` a new connection
    int (*on_disconnect)(xv_connection_t *);   // when connection will disconnect
    int (*on_send_data_done)(xv_message_t *);  // when write user packet to kernel socket buffer
    int (*cleanup)(void *);                    // cleanup user's packet
    uint64_t (*get_packet_id)(void *);         // get user packet's id
} xv_server_handle_t;

xv_server_t *xv_server_init(xv_server_config_t config);
int xv_server_add_listen(xv_server_t *server, const char *addr, int port, xv_server_handle_t handle);
int xv_server_start(xv_server_t *server);
int xv_server_run(xv_server_t *server);
int xv_server_stop(xv_server_t *server);
void xv_server_destroy(xv_server_t *server);

#ifdef __cplusplus
}
#endif

#endif // XV_SERVER_H_

