/* Copyright (C) 2016 Intel Corporation
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
/** Header file to include items needs by cmocka libraries */
#ifndef __UTEST_CMOCKA_H__
# define __UTEST_CMOCKA_H__

# include <stdarg.h>
# include <stddef.h>
# include <setjmp.h>
# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

# include <cmocka.h>
# include <pmix.h>

int __wrap_PMIx_Init(pmix_proc_t *proc, pmix_info_t info[], size_t ninfo);
int __wrap_PMIx_Get(const pmix_proc_t *proc, const char key[],
		    const pmix_info_t info[], size_t ninfo, pmix_value_t **val);
int __wrap_PMIx_Publish(const pmix_info_t info[], size_t ninfo);
int __wrap_PMIx_Lookup(pmix_pdata_t data[], size_t ndata,
		       const pmix_info_t info[], size_t ninfo);
int __wrap_PMIx_Fence(const pmix_proc_t procs[], size_t nprocs,
		      const pmix_info_t info[], size_t ninfo);
int __wrap_PMIx_Unpublish(char **keys, const pmix_info_t info[], size_t ninfo);
void __wrap_PMIx_Register_event_handler(pmix_status_t codes[], size_t ncodes,
					pmix_info_t info[], size_t ninfo,
					pmix_notification_fn_t evhdlr,
					pmix_evhdlr_reg_cbfunc_t cbfunc,
					void *cbdata);

#define expect_pmix_get(type, value)                        \
	do {                                                \
		will_return(__wrap_PMIx_Get, type);         \
		will_return(__wrap_PMIx_Get, value);        \
		will_return(__wrap_PMIx_Get, PMIX_SUCCESS); \
	} while (0)

#define expect_pmix_lookup(type, value)                        \
	do {                                                   \
		will_return(__wrap_PMIx_Lookup, type);         \
		will_return(__wrap_PMIx_Lookup, value);        \
		will_return(__wrap_PMIx_Lookup, PMIX_SUCCESS); \
	} while (0)

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __UTEST_CMOCKA_H__ */