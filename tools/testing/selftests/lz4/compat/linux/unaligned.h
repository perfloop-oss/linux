/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SELFTEST_LINUX_UNALIGNED_H
#define SELFTEST_LINUX_UNALIGNED_H

#include <linux/types.h>
#include <string.h>

#define get_unaligned(pointer) ({ \
	__typeof__(*(pointer) + 0) unaligned_value; \
	memcpy(&unaligned_value, (pointer), sizeof(unaligned_value)); \
	unaligned_value; \
})

#define put_unaligned(new_value, pointer) do { \
	__typeof__(*(pointer)) unaligned_value = (new_value); \
	memcpy((pointer), &unaligned_value, sizeof(unaligned_value)); \
} while (0)

static __always_inline uint16_t get_unaligned_le16(const void *pointer)
{
	const uint8_t *bytes = pointer;

	return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static __always_inline void put_unaligned_le16(uint16_t value, void *pointer)
{
	uint8_t *bytes = pointer;

	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
}

#endif /* SELFTEST_LINUX_UNALIGNED_H */
