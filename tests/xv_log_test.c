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

int main(int argc, char *argv[])
{
    xv_set_log_level(XV_LOG_DEBUG);

    xv_log_debug("this is a DEBUG log... %d", 0);
    xv_log_info("this is a INFO log... %d", 1);
    xv_log_warn("this is a WARN log... %d", 2);
    xv_log_error("this is a ERROR log... %d", 3);

    xv_set_log_filename("xv_log_test.log");

    xv_log_debug("this is a DEBUG log... %d", 0);
    xv_log_info("this is a INFO log... %d", 1);
    xv_log_warn("this is a WARN log... %d", 2);
    xv_log_error("this is a ERROR log... %d", 3);

    return EXIT_SUCCESS;
}

