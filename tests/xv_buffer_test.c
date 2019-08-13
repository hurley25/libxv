/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_log_test.c 08/09/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "xv_test.h"
#include "xv_buffer.h"

// +---------+-------------------+------------------+
// |         |   readable bytes  |  writable bytes  |
// |         |     (CONTENT)     |                  |
// +---------+-------------------+------------------+
// |         |                   |                  |
// 0 <= read_idx      <=     write_idx     <=     size
// 

int main(int argc, char *argv[])
{
    // xv_set_log_level(XV_LOG_DEBUG);
    xv_buffer_t *buffer = xv_buffer_init(16);
    ASSERT(buffer);
    ASSERT(buffer->buf);
    ASSERT(buffer->read_idx == 0);
    ASSERT(buffer->write_idx == 0);
    ASSERT(buffer->size == 16);

    ASSERT(xv_buffer_read_begin(buffer) == buffer->buf);
    ASSERT(xv_buffer_write_begin(buffer) == buffer->buf);

    ASSERT(xv_buffer_writeable_size(buffer) == 16);

    const char *str = "hello xv!";
    int len = strlen(str);

    xv_buffer_write_data(buffer, str, len);
    ASSERT(xv_buffer_readable_size(buffer) == len);
    ASSERT(xv_buffer_writeable_size(buffer) == 16 - len);

    char read_buf[32];
    int nread = xv_buffer_read_data(buffer, read_buf, 32);
    ASSERT(nread == len);

    ASSERT(buffer->read_idx == 0);
    ASSERT(buffer->write_idx == 0);

    ASSERT(xv_buffer_read_begin(buffer) == buffer->buf);
    ASSERT(xv_buffer_write_begin(buffer) == buffer->buf);

    xv_buffer_write_data(buffer, str, len);
    ASSERT(xv_buffer_readable_size(buffer) == len);
    ASSERT(xv_buffer_writeable_size(buffer) == 16 - len);

    xv_buffer_ensure_writeable_size(buffer, len);

    memcpy(xv_buffer_write_begin(buffer), str, len);
    xv_buffer_incr_write_index(buffer, len);
    ASSERT(xv_buffer_readable_size(buffer) == len * 2);

    ASSERT(memcmp(xv_buffer_read_begin(buffer), str, len) == 0);
    xv_buffer_incr_read_index(buffer, len);
    ASSERT(memcmp(xv_buffer_read_begin(buffer), str, len) == 0);

    xv_buffer_incr_read_index(buffer, len);
    ASSERT(buffer->read_idx == 0);
    ASSERT(buffer->write_idx == 0);

    xv_buffer_destroy(buffer);

    return EXIT_SUCCESS;
}

