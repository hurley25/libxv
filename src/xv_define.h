/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_define.h 2019/08/08 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_DEFINE_H_
#define XV_DEFINE_H_

#ifdef __cplusplus
extern "C" {
#endif

// xv memory alloc/free function define
#ifndef xv_malloc
#define xv_malloc malloc
#define xv_realloc realloc
#define xv_free free
#endif

// xv return code
#define XV_OK     0
#define XV_AGAIN  1
#define XV_ERR   -1

// xv event type
#define XV_NONE 0x0
#define XV_READ 0x1
#define XV_WRITE 0x2
#define XV_ALL_EVENT (XV_READ | XV_WRITE)

const char *xv_event_to_str(int event);

#define xv_memory_barriers() __sync_synchronize()

#ifdef __cplusplus
}
#endif

#endif // XV_DEFINE_H_

