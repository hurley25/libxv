/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_buffer.c 08/13/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_buffer.h"

#include <stdlib.h>
#include <string.h>

#include "xv_define.h"
#include "xv_log.h"

#define XV_BUFFER_MOVE_CHECK_SIZE (1024 * 2)

xv_buffer_t *xv_buffer_init(int size)
{
    xv_buffer_t *buffer = xv_malloc(sizeof(xv_buffer_t));
    buffer->buf = (char *)xv_malloc(size);
    buffer->read_idx = 0;
    buffer->write_idx = 0;
    buffer->size = size;

    return buffer;
}

void xv_buffer_destroy(xv_buffer_t *buffer)
{
    xv_free(buffer->buf);
    xv_free(buffer);
}

void xv_buffer_clear(xv_buffer_t *buffer)
{
    buffer->read_idx = 0;
    buffer->write_idx = 0;
}

static void xv_buffer_try_move(xv_buffer_t *buffer)
{
    int nread = xv_buffer_readable_size(buffer);
    if (nread == 0) {
        buffer->read_idx = 0;
        buffer->write_idx = 0;
    }
    if (buffer->read_idx > buffer->size / 2 && buffer->read_idx > XV_BUFFER_MOVE_CHECK_SIZE) {

        xv_log_debug("buffer data move, buffer->read_idx: %d, buffer->size: %d, readable size: %d", buffer->read_idx,  buffer->size, nread);

        int nread = xv_buffer_readable_size(buffer);
        memmove(buffer->buf, buffer->buf + buffer->read_idx, nread);
        buffer->read_idx = 0;
        buffer->write_idx = nread;
    }
}

int xv_buffer_readable_size(xv_buffer_t *buffer)
{
    return buffer->write_idx - buffer->read_idx;
}

int xv_buffer_writeable_size(xv_buffer_t *buffer)
{
    return buffer->size - buffer->write_idx;
}

void xv_buffer_ensure_writeable_size(xv_buffer_t *buffer, int valid_size)
{
    if (xv_buffer_writeable_size(buffer) < valid_size) {
        int new_size = buffer->size + valid_size;

        xv_log_debug("resize buffer size, old size: %d, new size: %d", buffer->size, new_size);

        buffer->buf = xv_realloc(buffer->buf, new_size);
        buffer->size = new_size;
    }
}

char *xv_buffer_read_begin(xv_buffer_t *buffer)
{
    return buffer->buf + buffer->read_idx;
}

char *xv_buffer_write_begin(xv_buffer_t *buffer)
{
    return buffer->buf + buffer->write_idx;
}

// Note: After calling this function, you need to call `xv_buffer_read_begin` again to get a new read address
void xv_buffer_incr_read_index(xv_buffer_t *buffer, int size)
{
    buffer->read_idx += size;
    xv_buffer_try_move(buffer);
}

// Note: After calling this function, you need to call `xv_buffer_write_begin` again to get a new read address
void xv_buffer_incr_write_index(xv_buffer_t *buffer, int size)
{
    buffer->write_idx += size;
}

int xv_buffer_read_data(xv_buffer_t *buffer, char *dst, int len)
{
    int nread = xv_buffer_readable_size(buffer);
    if (len > nread) {
        len = nread;
    }
    char *src = buffer->buf + buffer->read_idx;
    memcpy(dst, src, len);
    buffer->read_idx += len;
    xv_buffer_try_move(buffer);

    return len;
}

int xv_buffer_write_data(xv_buffer_t *buffer, const char *src, int len)
{
    xv_buffer_ensure_writeable_size(buffer, len);
    char *dst = buffer->buf + buffer->write_idx;
    memcpy(dst, src, len);
    buffer->write_idx += len;
    
    return len;
}

