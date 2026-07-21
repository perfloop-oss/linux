# SPDX-License-Identifier: GPL-2.0

OVERRIDE_TARGETS := 1
TEST_GEN_PROGS := lz4_destsize_bench

CFLAGS += -O2 -Wall -Wextra -Werror -std=gnu11

include ../lib.mk

LZ4_COMPAT_INCLUDES := -I$(CURDIR)/compat -I$(top_srcdir)/include
LZ4_SOURCES := $(top_srcdir)/lib/lz4/lz4_compress.c \
	$(top_srcdir)/lib/lz4/lz4_decompress.c
LZ4_HEADERS := $(top_srcdir)/lib/lz4/lz4defs.h \
	$(top_srcdir)/include/linux/lz4.h \
	$(wildcard $(CURDIR)/compat/linux/*.h)

$(TEST_GEN_PROGS): lz4_destsize_bench.c $(LZ4_SOURCES) $(LZ4_HEADERS)
	$(call msg,CC,,$@)
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) $(LZ4_COMPAT_INCLUDES) \
		-DCONFIG_64BIT -D__LITTLE_ENDIAN \
		lz4_destsize_bench.c $(LZ4_SOURCES) $(LDLIBS) -o $@

check-reset-path: $(TEST_GEN_PROGS)
	$(Q)objdump -d $(TEST_GEN_PROGS) | awk '\
		/<LZ4_compress_destSize>:/ { in_dest = 1; in_fast = 0; next } \
		/<LZ4_compress_fast_extState>:/ { in_dest = 0; in_fast = 1; next } \
		/^[[:space:]]*[[:xdigit:]]+ <.*>:/ { in_dest = 0; in_fast = 0 } \
		in_dest && /LZ4_compress_fast_extState/ { delegated = 1 } \
		in_fast && /memset@plt/ { reset = 1 } \
		END { if (!delegated || !reset) exit 1; print "full-output reset delegation: PASS" }'

.PHONY: check-reset-path
