// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <linux/lz4.h>

#define SMALL_INPUT_SIZE 64U
#define PAGE_INPUT_SIZE 4096U

static volatile uint64_t benchmark_sink;

struct lz4_buffers {
	char *src;
	char *dst;
	char *decoded;
	void *workmem;
	size_t input_size;
	int destination_capacity;
};

static uint32_t next_random(uint32_t *state)
{
	uint32_t value = *state;

	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	*state = value;
	return value;
}

static void fill_input(char *buffer, size_t size, uint32_t seed, bool repetitive)
{
	uint32_t state = seed;
	size_t index;

	for (index = 0; index < size; index++) {
		if (repetitive)
			buffer[index] = (char)((index * 17U + seed) & 0x0fU);
		else
			buffer[index] = (char)next_random(&state);
	}
}

static int allocate_buffers(struct lz4_buffers *buffers, size_t input_size)
{
	int destination_capacity = LZ4_compressBound(input_size);

	if (destination_capacity <= 0)
		return -1;

	memset(buffers, 0, sizeof(*buffers));
	buffers->src = malloc(input_size);
	buffers->dst = malloc((size_t)destination_capacity);
	buffers->decoded = malloc(input_size);
	buffers->workmem = malloc(LZ4_MEM_COMPRESS);
	if (!buffers->src || !buffers->dst || !buffers->decoded ||
	    !buffers->workmem) {
		free(buffers->workmem);
		free(buffers->decoded);
		free(buffers->dst);
		free(buffers->src);
		memset(buffers, 0, sizeof(*buffers));
		return -1;
	}

	buffers->input_size = input_size;
	buffers->destination_capacity = destination_capacity;
	return 0;
}

static void free_buffers(struct lz4_buffers *buffers)
{
	free(buffers->workmem);
	free(buffers->decoded);
	free(buffers->dst);
	free(buffers->src);
	memset(buffers, 0, sizeof(*buffers));
}

static int test_full_output_route(size_t input_size, uint32_t seed,
				  bool repetitive)
{
	struct lz4_buffers destination = { };
	struct lz4_buffers fast = { };
	int source_size = (int)input_size;
	int destination_size;
	int fast_size;
	int decoded_size;
	int result = -1;

	if (allocate_buffers(&destination, input_size) ||
	    allocate_buffers(&fast, input_size))
		goto out;

	fill_input(destination.src, input_size, seed, repetitive);
	memset(destination.workmem, 0xa5, LZ4_MEM_COMPRESS);
	destination_size = LZ4_compress_destSize(destination.src, destination.dst,
						&source_size,
						destination.destination_capacity,
						destination.workmem);
	if (destination_size <= 0 || source_size != (int)input_size)
		goto out;

	memset(fast.workmem, 0x5a, LZ4_MEM_COMPRESS);
	fast_size = LZ4_compress_fast(destination.src, fast.dst,
				      (int)input_size,
				      fast.destination_capacity, 1, fast.workmem);
	if (fast_size != destination_size ||
	    memcmp(destination.dst, fast.dst, (size_t)destination_size))
		goto out;

	decoded_size = LZ4_decompress_safe(destination.dst, destination.decoded,
					   destination_size, (int)input_size);
	if (decoded_size != (int)input_size ||
	    memcmp(destination.src, destination.decoded, input_size))
		goto out;

	result = 0;
out:
	free_buffers(&fast);
	free_buffers(&destination);
	return result;
}

static int test_limited_output_route(size_t input_size, uint32_t seed,
				     bool repetitive)
{
	struct lz4_buffers dirty = { };
	struct lz4_buffers zeroed = { };
	int target_size = (int)(input_size / 2U);
	int dirty_source_size = (int)input_size;
	int zeroed_source_size = (int)input_size;
	int dirty_size;
	int zeroed_size;
	int decoded_size;
	int result = -1;

	if (allocate_buffers(&dirty, input_size) ||
	    allocate_buffers(&zeroed, input_size))
		goto out;

	fill_input(dirty.src, input_size, seed, repetitive);
	memset(dirty.workmem, 0xa5, LZ4_MEM_COMPRESS);
	dirty_size = LZ4_compress_destSize(dirty.src, dirty.dst,
					  &dirty_source_size, target_size,
					  dirty.workmem);
	if (dirty_size <= 0 || dirty_size > target_size ||
	    dirty_source_size <= 0 || dirty_source_size > (int)input_size)
		goto out;

	memset(zeroed.workmem, 0, LZ4_MEM_COMPRESS);
	zeroed_size = LZ4_compress_destSize(dirty.src, zeroed.dst,
					   &zeroed_source_size, target_size,
					   zeroed.workmem);
	if (zeroed_size != dirty_size ||
	    zeroed_source_size != dirty_source_size ||
	    memcmp(dirty.dst, zeroed.dst, (size_t)dirty_size))
		goto out;

	decoded_size = LZ4_decompress_safe(dirty.dst, dirty.decoded, dirty_size,
					   (int)input_size);
	if (decoded_size != dirty_source_size ||
	    memcmp(dirty.src, dirty.decoded, (size_t)dirty_source_size))
		goto out;

	result = 0;
out:
	free_buffers(&zeroed);
	free_buffers(&dirty);
	return result;
}

static int run_selftest(bool verbose)
{
	static const size_t input_sizes[] = {
		SMALL_INPUT_SIZE,
		PAGE_INPUT_SIZE,
	};
	size_t index;
	int repetitive;

	if (LZ4_MEM_COMPRESS != 16416) {
		fprintf(stderr, "unexpected LZ4 workmem size: %zu\n",
			(size_t)LZ4_MEM_COMPRESS);
		return -1;
	}

	for (index = 0; index < sizeof(input_sizes) / sizeof(input_sizes[0]);
	     index++) {
		for (repetitive = 0; repetitive <= 1; repetitive++) {
			uint32_t seed = 0x9e3779b9U + (uint32_t)input_sizes[index] +
					(uint32_t)repetitive;

			if (test_full_output_route(input_sizes[index], seed,
						   repetitive) ||
			    test_limited_output_route(input_sizes[index], seed,
						      repetitive)) {
				fprintf(stderr,
					"LZ4 destination-size test failed for %zu-byte %s input\n",
					input_sizes[index],
					repetitive ? "repetitive" : "random");
				return -1;
			}
		}
	}

	if (verbose) {
		puts("lz4-destsize-selftest: PASS");
		puts("full-output bytes and consumed size match fast path: PASS");
		puts("limited-output dirty-workmem prefix round-trip: PASS");
	}
	return 0;
}

static int pin_to_one_cpu(void)
{
	cpu_set_t available;
	cpu_set_t selected;
	int cpu;

	CPU_ZERO(&available);
	if (sched_getaffinity(0, sizeof(available), &available)) {
		fprintf(stderr, "sched_getaffinity: %s\n", strerror(errno));
		return -1;
	}

	for (cpu = 0; cpu < CPU_SETSIZE; cpu++) {
		if (CPU_ISSET(cpu, &available))
			break;
	}
	if (cpu == CPU_SETSIZE) {
		fputs("no available CPU for benchmark\n", stderr);
		return -1;
	}

	CPU_ZERO(&selected);
	CPU_SET(cpu, &selected);
	if (sched_setaffinity(0, sizeof(selected), &selected)) {
		fprintf(stderr, "sched_setaffinity: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static uint64_t monotonic_time_ns(void)
{
	struct timespec timestamp;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp))
		return 0;
	return (uint64_t)timestamp.tv_sec * UINT64_C(1000000000) +
		(uint64_t)timestamp.tv_nsec;
}

static int run_calls(struct lz4_buffers *buffers, unsigned int iterations,
		     uint32_t seed, uint64_t *checksum)
{
	uint32_t state = seed;
	unsigned int index;

	for (index = 0; index < iterations; index++) {
		int source_size = (int)buffers->input_size;
		int compressed_size;
		size_t changed_index;

		state = next_random(&state);
		changed_index = (size_t)state % buffers->input_size;
		buffers->src[changed_index] ^= (char)state;
		compressed_size = LZ4_compress_destSize(buffers->src, buffers->dst,
							     &source_size,
							     buffers->destination_capacity,
							     buffers->workmem);
		if (compressed_size <= 0 || source_size != (int)buffers->input_size)
			return -1;
		*checksum += (uint8_t)buffers->dst[(size_t)state %
					      (size_t)compressed_size];
		*checksum += (uint64_t)compressed_size;
	}
	return 0;
}

static int measure_shape(size_t input_size, unsigned int iterations,
			 double *nanoseconds_per_call)
{
	struct lz4_buffers buffers = { };
	uint64_t checksum = 0;
	uint64_t start;
	uint64_t finish;
	int result = -1;

	if (allocate_buffers(&buffers, input_size))
		goto out;
	fill_input(buffers.src, input_size,
		   (uint32_t)monotonic_time_ns() | UINT32_C(1), false);
	memset(buffers.workmem, 0xa5, LZ4_MEM_COMPRESS);
	if (run_calls(&buffers, 2000U,
		      (uint32_t)monotonic_time_ns() | UINT32_C(1), &checksum))
		goto out;

	start = monotonic_time_ns();
	if (!start || run_calls(&buffers, iterations,
				(uint32_t)start | UINT32_C(1), &checksum))
		goto out;
	finish = monotonic_time_ns();
	if (finish <= start)
		goto out;

	benchmark_sink ^= checksum;
	*nanoseconds_per_call = (double)(finish - start) / (double)iterations;
	result = 0;
out:
	free_buffers(&buffers);
	return result;
}

static int run_benchmark(void)
{
	double page_ns_per_call;
	double small_ns_per_call;

	if (run_selftest(false) || pin_to_one_cpu())
		return -1;
	if (measure_shape(PAGE_INPUT_SIZE, 100000U, &page_ns_per_call) ||
	    measure_shape(SMALL_INPUT_SIZE, 400000U, &small_ns_per_call)) {
		fputs("LZ4 destination-size benchmark failed\n", stderr);
		return -1;
	}

	printf("{\"metric\":\"lz4_destsize_full_4k_ns_per_call\",\"value\":%.6f}\n",
	       page_ns_per_call);
	printf("{\"metric\":\"lz4_destsize_full_64b_ns_per_call\",\"value\":%.6f}\n",
	       small_ns_per_call);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 1 || (argc == 2 && !strcmp(argv[1], "--check")))
		return run_selftest(true) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (argc == 2 && !strcmp(argv[1], "--benchmark"))
		return run_benchmark() ? EXIT_FAILURE : EXIT_SUCCESS;

	fprintf(stderr, "usage: %s [--check|--benchmark]\n", argv[0]);
	return EXIT_FAILURE;
}
