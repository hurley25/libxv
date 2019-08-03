/**
 * (C) 2007-2019 XiYouF4 Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: 1.0: xv_test.h 2019/08/09 $
 *
 * Authors:
 *   hurley25 <liuhuan1992@gmail.com>
 */

#ifndef XV_TEST_H_
#define XV_TEST_H_

#include <assert.h>

#include "xv.h"
#include "xv_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT(conditon) do { assert(conditon); } while(0)

#define CHECK(conditon, info) do { if (!(conditon)) { perror(info); assert(0); } } while(0)

#ifdef __cplusplus
}
#endif

#endif // XV_TEST_H_

