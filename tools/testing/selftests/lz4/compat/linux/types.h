/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SELFTEST_LINUX_TYPES_H
#define SELFTEST_LINUX_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

#ifndef __always_inline
#define __always_inline inline __attribute__((__always_inline__))
#endif

#endif /* SELFTEST_LINUX_TYPES_H */
