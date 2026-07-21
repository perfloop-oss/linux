/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SELFTEST_LINUX_KERNEL_H
#define SELFTEST_LINUX_KERNEL_H

#ifndef likely
#define likely(expression) __builtin_expect(!!(expression), 1)
#endif
#ifndef unlikely
#define unlikely(expression) __builtin_expect(!!(expression), 0)
#endif
#ifndef BUILD_BUG_ON
#define BUILD_BUG_ON(condition) _Static_assert(!(condition), "BUILD_BUG_ON")
#endif
#ifndef min
#define min(left, right) ((left) < (right) ? (left) : (right))
#endif

#endif /* SELFTEST_LINUX_KERNEL_H */
