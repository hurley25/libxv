/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_buffer.h 08/13/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_BUFFER_H_
#define XV_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

// +---------+-------------------+------------------+
// |         |   readable bytes  |  writable bytes  |
// |         |     (CONTENT)     |                  |
// +---------+-------------------+------------------+
// |         |                   |                  |
// 0 <= read_idx      <=     write_idx     <=     size

typedef struct xv_buffer_t {
    char *buf;
    int size;
    int read_idx;
    int write_idx;
} xv_buffer_t;

xv_buffer_t *xv_buffer_init(int size);
void xv_buffer_destroy(xv_buffer_t *buffer);

void xv_buffer_clear(xv_buffer_t *buffer);

int xv_buffer_readable_size(xv_buffer_t *buffer);
int xv_buffer_writeable_size(xv_buffer_t *buffer);

void xv_buffer_ensure_writeable_size(xv_buffer_t *buffer, int valid_size);

char *xv_buffer_read_begin(xv_buffer_t *buffer);
char *xv_buffer_write_begin(xv_buffer_t *buffer);

// Note: After calling this function, you need to call `xv_buffer_read_begin` again to get a new read address
void xv_buffer_incr_read_index(xv_buffer_t *buffer, int size);

// Note: After calling this function, you need to call `xv_buffer_write_begin` again to get a new read address
void xv_buffer_incr_write_index(xv_buffer_t *buffer, int size);

// auto incr index function
int xv_buffer_read_data(xv_buffer_t *buffer, char *dst, int len);
int xv_buffer_write_data(xv_buffer_t *buffer, const char *src, int len);

#ifdef __cplusplus
}
#endif

#endif // XV_BUFFER_H_

