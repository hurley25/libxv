/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_log.h 08/09/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_LOG_H_
#define XV_LOG_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum xv_log_level {
    XV_LOG_DEBUG = 0,
    XV_LOG_INFO = 1,
    XV_LOG_WARNING = 2,
    XV_LOG_ERROR = 3,
} xv_log_level_t;

extern xv_log_level_t xv_curr_log_level;

#define xv_log_debug(args ...) xv_log(XV_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, args)
#define xv_log_info(args ...) xv_log(XV_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, args)
#define xv_log_warn(args ...) xv_log(XV_LOG_WARNING, __FILE__, __LINE__, __FUNCTION__, args)
#define xv_log_error(args ...) xv_log(XV_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, args)

#define xv_log_errno_error(msg) do {\
    char errbuf[128];\
        strerror_r(errno, errbuf, sizeof(errbuf));\
        xv_log_error(msg ": %s", errbuf);\
} while(0)


// default to stderr
void xv_set_log_file(FILE *pf);
int xv_set_log_filename(const char *filename);

// set log level
void xv_set_log_level(xv_log_level_t level);

// log something
void xv_log(xv_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // XV_LOG_H_

