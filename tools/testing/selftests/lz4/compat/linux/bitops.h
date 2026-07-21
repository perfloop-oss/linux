/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SELFTEST_LINUX_BITOPS_H
#define SELFTEST_LINUX_BITOPS_H

#include <linux/types.h>

#ifndef likely
#define likely(expression) __builtin_expect(!!(expression), 1)
#endif
#ifndef unlikely
#define unlikely(expression) __builtin_expect(!!(expression), 0)
#endif

static __always_inline unsigned long __ffs(unsigned long word)
{
	return __builtin_ctzl(word);
}

static __always_inline unsigned long __fls(unsigned long word)
{
	return (sizeof(word) * 8U - 1U) - __builtin_clzl(word);
}

#endif /* SELFTEST_LINUX_BITOPS_H */
