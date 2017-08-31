/* Copyright (C) 2016-2017 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the
 *    documentation and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 *  4. All publications or advertising materials mentioning features or use of
 *     this software are asked, but not required, to acknowledge that it was
 *     developed by Intel Corporation and credit the contributors.
 *
 * 5. Neither the name of Intel Corporation, nor the name of any Contributor
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CRT_COMMON_H__
#define __CRT_COMMON_H__

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <byteswap.h>
#include <cart/api.h>
#include <pouch/debug.h>
#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>

/* Get the current time using a monotonic timer
 * param[out] ts A timespec structure for the result
 */
static inline int _crt_gettime(struct timespec *ts)
{
	clock_serv_t cclock;
	mach_timespec_t mts;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
	return 0;
}
#else
#include <time.h>
/* Get the current time using a monotonic timer
 * param[out] ts A timespec structure for the result
 */
#define _crt_gettime(ts) clock_gettime(CLOCK_MONOTONIC, ts)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* memory allocating macros */

#define MEM_DBG		(crt_mem_logfac | CLOG_DBG)

#define C_ALLOC(ptr, size)						       \
	do {								       \
		(ptr) = (__typeof__(ptr))calloc(1, (size));		       \
		if ((ptr) != NULL) {					       \
			crt_log(MEM_DBG, "%s:%d, alloc #ptr : %d at %p.\n",    \
				__FILE__, __LINE__, (int)(size), ptr);	       \
			break;						       \
		}							       \
		C_ERROR("%s:%d, out of memory(tried to alloc '" #ptr "' = %d)",\
			__FILE__, __LINE__, (int)(size));		       \
	} while (0)

static inline void *
crt_c_realloc(void *ptrptr, size_t size)
{
	void *tmp_ptr;

	tmp_ptr = realloc(ptrptr, size);
	if (size == 0 || tmp_ptr != NULL) {
		crt_log(MEM_DBG, "realloc #ptr : %d at %p.\n",
			(int) size, ptrptr);

		return tmp_ptr;
	}
	C_ERROR("out of memory (tried to realloc \" #ptr \" = %d)",
		(int)(size));

	return tmp_ptr;
}
/* memory reallocation */
#define C_REALLOC(ptr, size)						\
	(__typeof__(ptr)) crt_c_realloc((ptr), (size))

# define C_FREE(ptr, size)						\
	do {								\
		crt_log(MEM_DBG, "%s:%d, free #ptr : %d at %p.\n",	\
			__FILE__, __LINE__, (int)(size), (ptr));	\
		free(ptr);						\
		(ptr) = NULL;						\
	} while (0)

#define C_ALLOC_PTR(ptr)        C_ALLOC((ptr), sizeof *(ptr))
#define C_FREE_PTR(ptr)         C_FREE((ptr), sizeof *(ptr))

#define C_GOTO(label, rc)       do { ((void)(rc)); goto label; } while (0)

#define CRT_GOLDEN_RATIO_PRIME_64	0xcbf29ce484222325ULL
#define CRT_GOLDEN_RATIO_PRIME_32	0x9e370001UL

static inline uint64_t
crt_u64_hash(uint64_t val, unsigned int bits)
{
	uint64_t hash = val;

	hash *= CRT_GOLDEN_RATIO_PRIME_64;
	return hash >> (64 - bits);
}

static inline uint32_t
crt_u32_hash(uint64_t key, unsigned int bits)
{
	return (CRT_GOLDEN_RATIO_PRIME_32 * key) >> (32 - bits);
}

uint64_t crt_hash_mix64(uint64_t key);
uint32_t crt_hash_mix96(uint32_t a, uint32_t b, uint32_t c);

/** consistent hash search */
unsigned int crt_chash_srch_u64(uint64_t *hashes, unsigned int nhashes,
				 uint64_t value);

/** djb2 hash a string to a uint32_t value */
uint32_t crt_hash_string_u32(const char *string, unsigned int len);
/** murmur hash (64 bits) */
uint64_t crt_hash_murmur64(const unsigned char *key, unsigned int key_len,
			    unsigned int seed);

#define LOWEST_BIT_SET(x)       ((x) & ~((x) - 1))

static inline unsigned int
crt_power2_nbits(unsigned int val)
{
	unsigned int shift;

	for (shift = 1; (val >> shift) != 0; shift++);

	return val == LOWEST_BIT_SET(val) ? shift - 1 : shift;
}

int crt_rank_list_dup(crt_rank_list_t **dst, const crt_rank_list_t *src,
		      bool input);
int crt_rank_list_dup_sort_uniq(crt_rank_list_t **dst,
				const crt_rank_list_t *src, bool input);
void crt_rank_list_filter(crt_rank_list_t *src_set, crt_rank_list_t *dst_set,
			  bool input, bool exclude);
crt_rank_list_t *crt_rank_list_alloc(uint32_t size);
crt_rank_list_t *crt_rank_list_realloc(crt_rank_list_t *ptr, uint32_t size);
void crt_rank_list_free(crt_rank_list_t *rank_list);
void crt_rank_list_copy(crt_rank_list_t *dst, crt_rank_list_t *src, bool input);
void crt_rank_list_sort(crt_rank_list_t *rank_list);
bool crt_rank_list_find(crt_rank_list_t *rank_list, crt_rank_t rank, int *idx);
int crt_rank_list_del(crt_rank_list_t *rank_list, crt_rank_t rank);
bool crt_rank_list_identical(crt_rank_list_t *rank_list1,
			     crt_rank_list_t *rank_list2, bool input);
bool crt_rank_in_rank_list(crt_rank_list_t *rank_list, crt_rank_t rank,
			   bool input);
int crt_idx_in_rank_list(crt_rank_list_t *rank_list, crt_rank_t rank,
			 uint32_t *idx, bool input);
int crt_rank_list_append(crt_rank_list_t *rank_list, crt_rank_t rank);
int crt_rank_list_dump(crt_rank_list_t *rank_list, crt_string_t name);
int crt_sgl_init(crt_sg_list_t *sgl, unsigned int nr);
void crt_sgl_fini(crt_sg_list_t *sgl, bool free_iovs);
void crt_getenv_bool(const char *env, bool *bool_val);
void crt_getenv_int(const char *env, unsigned *int_val);


#if !defined(container_of)
/* given a pointer @ptr to the field @member embedded into type (usually
 *  * struct) @type, return pointer to the embedding instance of @type. */
# define container_of(ptr, type, member)		\
	((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))
#endif

#ifndef offsetof
# define offsetof(typ, memb)	((long)((char *)&(((typ *)0)->memb)))
#endif

#define C_ALIGNUP(x, a) (((x) + (a - 1)) & ~(a - 1))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef MIN
# define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef min_t
#define min_t(type, x, y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x : __y; })
#endif
#ifndef max_t
#define max_t(type, x, y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x : __y; })
#endif

/* byte swapper */
#define C_SWAP16(x)	bswap_16(x)
#define C_SWAP32(x)	bswap_32(x)
#define C_SWAP64(x)	bswap_64(x)
#define C_SWAP16S(x)	do { *(x) = C_SWAP16(*(x)); } while (0)
#define C_SWAP32S(x)	do { *(x) = C_SWAP32(*(x)); } while (0)
#define C_SWAP64S(x)	do { *(x) = C_SWAP64(*(x)); } while (0)

static inline int
crt_errno2cer(int err)
{
	switch (err) {
	case 0:		return 0;
	case EPERM:
	case EACCES:	return -CER_NO_PERM;
	case ENOMEM:	return -CER_NOMEM;
	case EDQUOT:
	case ENOSPC:	return -CER_NOSPACE;
	case EEXIST:	return -CER_EXIST;
	case ENOENT:	return -CER_NONEXIST;
	case ECANCELED:	return -CER_CANCELED;
	default:	return -CER_MISC;
	}
	return 0;
}

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC  1000000000
#endif
#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC 1000000
#endif
#ifndef NSEC_PER_USEC
#define NSEC_PER_USEC 1000
#endif

/* timing utilities */
static inline int
crt_gettime(struct timespec *t)
{
	int	rc;

	rc = _crt_gettime(t);
	if (rc != 0) {
		C_ERROR("clock_gettime failed, rc: %d, errno %d(%s).\n",
			rc, errno, strerror(errno));
		rc = crt_errno2cer(errno);
	}

	return rc;
}

/* Calculate t2 - t1 in nanoseconds */
static inline int64_t
crt_timediff_ns(const struct timespec *t1, const struct timespec *t2)
{
	return ((t2->tv_sec - t1->tv_sec) * NSEC_PER_SEC) +
		t2->tv_nsec - t1->tv_nsec;
}

/* Calculate end - start as timespec. */
static inline struct timespec
crt_timediff(struct timespec start, struct timespec end)
{
	struct timespec		temp;

	if ((end.tv_nsec - start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = NSEC_PER_SEC + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}


	return temp;
}

/* Calculate remaining time in ns */
static inline int64_t
crt_timeleft_ns(const struct timespec *expiration)
{
	struct timespec		now;
	int64_t			ns;

	crt_gettime(&now);

	ns = crt_timediff_ns(&now, expiration);

	if (ns <= 0)
		return 0;

	return ns;
}

/* calculate the number in us after \param sec_diff second */
static inline uint64_t
crt_timeus_secdiff(unsigned sec_diff)
{
	struct timespec		now;
	uint64_t		us;

	crt_gettime(&now);
	us = (now.tv_sec + sec_diff) * 1e6 + now.tv_nsec / 1e3;

	return us;
}

/* Increment time by ns nanoseconds */
static inline void
crt_timeinc(struct timespec *now, uint64_t ns)
{
	now->tv_nsec += ns;
	now->tv_sec += now->tv_nsec / NSEC_PER_SEC;
	now->tv_nsec = now->tv_nsec % NSEC_PER_SEC;
}

static inline double
crt_time2ms(struct timespec t)
{
	return (double) t.tv_sec * 1e3 + (double) t.tv_nsec / 1e6;
}

static inline double
crt_time2us(struct timespec t)
{
	return (double) t.tv_sec * 1e6 + (double) t.tv_nsec / 1e3;
}

static inline double
crt_time2s(struct timespec t)
{
	return (double) t.tv_sec + (double) t.tv_nsec / 1e9;
}

#if defined(__cplusplus)
}
#endif

#endif /* __CRT_COMMON_H__ */