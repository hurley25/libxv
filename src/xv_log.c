/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_log.c 08/09/2019 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#include "xv_log.h"

#include <string.h>

#include <stdlib.h>
#include <stdarg.h>
#include <libgen.h>
#include <time.h>

#include <sys/time.h>

#include <unistd.h>
#include <sys/syscall.h>

#include "xv_define.h"

#define XV_LOG_LINE_MAX  1024

static FILE *xv_logger_fp = NULL;

xv_log_level_t xv_curr_log_level = XV_LOG_INFO;

static const char *xv_log_level_str(xv_log_level_t level)
{
    static const char *level_str[] = {"DEUBG", "INFO", "WARN", "ERROR"};
    if (level >= XV_LOG_DEBUG && level <= XV_LOG_ERROR) {
        return level_str[level];
    }

    return "UNKNOWN";
}

void xv_set_log_file(FILE *pf)
{
    if (pf) {
        xv_logger_fp = pf;
    }
}

static void xv_close_log_file(void)
{
    if (xv_logger_fp && xv_logger_fp != stderr) {
        fclose(xv_logger_fp);
    }
}

int xv_set_log_filename(const char *filename)
{
    xv_close_log_file();
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return XV_ERR;
    }
    xv_logger_fp = fp;

    // fclose log file when exit
    atexit(xv_close_log_file);

    return XV_OK;
}

void xv_set_log_level(xv_log_level_t level)
{
    xv_curr_log_level = level;
}

void xv_log(xv_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...)
{
    char logger_buf[XV_LOG_LINE_MAX];

    // level filter
    if (level < xv_curr_log_level) {
        return;
    }
    
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logger_buf, sizeof(logger_buf), fmt, ap);
    va_end(ap);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t time_s = tv.tv_sec;
    struct tm now_tm;
    localtime_r(&time_s, &now_tm);

    char datetime[64];
    int off = strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S.", &now_tm);
    snprintf(datetime + off, sizeof(datetime) - off, "%06d", (int)tv.tv_usec);

    int tid = syscall(SYS_gettid);

    if (!xv_logger_fp) {
        xv_logger_fp = stderr;
    }

    // [datetime] level func(file:line) [tid] ...
    fprintf(xv_logger_fp, "[%s] %s %s(%s:%d) [%d] %s\n",
            datetime, xv_log_level_str(level), func, basename((char *)file), line, tid, logger_buf);
}

