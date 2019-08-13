/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_loop_test.c 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xv_test.h"

#define SEND_STR "Hello libxv!"

void write_callback(xv_loop_t *loop, xv_io_t *io)
{
    ASSERT(xv_io_get_userdata(io) == loop);

    write(STDOUT_FILENO, SEND_STR, strlen(SEND_STR));

    // break here
    xv_loop_break(loop);
}

int main(int argc, char *argv[])
{
    xv_set_log_level(XV_LOG_DEBUG);

    xv_loop_t *loop = xv_loop_init(1024);
    ASSERT(loop != NULL);

    xv_io_t *io_write = xv_io_init(STDOUT_FILENO, XV_WRITE, write_callback);
    xv_io_set_userdata(io_write, loop);
    
    int ret = xv_io_start(loop, io_write);
    ASSERT(ret == XV_OK);

    // blockiong here
    xv_loop_run(loop);

    ret = xv_io_stop(loop, io_write);
    ASSERT(ret == XV_OK);
    ret = xv_io_destroy(io_write);
    ASSERT(ret == XV_OK);

    xv_loop_destroy(loop);

    return EXIT_SUCCESS;
}

