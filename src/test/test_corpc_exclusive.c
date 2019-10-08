/* Copyright (C) 2018-2019 Intel Corporation
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
/**
 * Basic CORPC test to check CRT_FLAG_RPC_EXCLUSIVE flag. Test assumes 5 ranks.
 * CORPC with 'shutdown' is sent to 3 ranks, 1,2 and 4.
 * Ranks0 and 3 are expected to not receive this call.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include "tests_common.h"

static int	g_do_shutdown;
static d_rank_t my_rank;

static int
corpc_aggregate(crt_rpc_t *src, crt_rpc_t *result, void *priv)
{
	return 0;
}

struct crt_corpc_ops corpc_set_ivns_ops = {
	.co_aggregate = corpc_aggregate,
};

static void
test_basic_corpc_hdlr(crt_rpc_t *rpc)
{
	int rc;

	D_DEBUG(DB_TEST, "Handler called\n");

	rc = crt_reply_send(rpc);
	assert(rc == 0);

	g_do_shutdown = 1;

	/* CORPC is not sent to those ranks */
	if (my_rank == 3 || my_rank == 0) {
		D_ERROR("CORPC was sent to wrong rank=%d\n", my_rank);
		assert(0);
	}

}

#define TEST_BASIC_CORPC 0xC1

#define TEST_CORPC_PREFWD_BASE 0x010000000
#define TEST_CORPC_PREFWD_VER  0

#define CRT_ISEQ_BASIC_CORPC	/* input fields */		 \
	((uint32_t)		(unused)		CRT_VAR)

#define CRT_OSEQ_BASIC_CORPC	/* output fields */		 \
	((uint32_t)		(unused)		CRT_VAR)

CRT_RPC_DECLARE(basic_corpc, CRT_ISEQ_BASIC_CORPC, CRT_OSEQ_BASIC_CORPC)
CRT_RPC_DEFINE(basic_corpc, CRT_ISEQ_BASIC_CORPC, CRT_OSEQ_BASIC_CORPC)

static void
corpc_response_hdlr(const struct crt_cb_info *info)
{
	g_do_shutdown = 1;
}

static struct crt_proto_rpc_format my_proto_rpc_fmt_basic_corpc[] = {
	{
		.prf_flags	= 0,
		.prf_req_fmt	= &CQF_basic_corpc,
		.prf_hdlr	= test_basic_corpc_hdlr,
		.prf_co_ops	= &corpc_set_ivns_ops,
	}
};

static struct crt_proto_format my_proto_fmt_basic_corpc = {
	.cpf_name = "my-proto-basic_corpc",
	.cpf_ver = TEST_CORPC_PREFWD_VER,
	.cpf_count = ARRAY_SIZE(my_proto_rpc_fmt_basic_corpc),
	.cpf_prf = &my_proto_rpc_fmt_basic_corpc[0],
	.cpf_base = TEST_CORPC_PREFWD_BASE,
};


int main(void)
{
	int		rc;
	crt_context_t	g_main_ctx;
	d_rank_list_t	membs;
	d_rank_t	memb_ranks[] = {1, 2, 4};
	crt_rpc_t	*rpc;
	uint32_t	grp_size;

	membs.rl_nr = 3;
	membs.rl_ranks = memb_ranks;

	rc = d_log_init();
	assert(rc == 0);

	rc = crt_init(NULL, CRT_FLAG_BIT_SERVER);
	assert(rc == 0);

	rc = crt_group_config_save(NULL, true);
	assert(rc == 0);

	rc = crt_group_size(NULL, &grp_size);
	assert(rc == 0);

	if (grp_size != 5) {
		D_ERROR("This test assumes 5 ranks\n");
		assert(0);
	}

	rc = crt_proto_register(&my_proto_fmt_basic_corpc);
	assert(rc == 0);

	rc = crt_context_create(&g_main_ctx);
	assert(rc == 0);

	rc =  crt_group_rank(NULL, &my_rank);

	if (my_rank == 0) {
		D_DEBUG(DB_TEST, "Rank 0 sending CORPC call\n");
		rc = crt_corpc_req_create(g_main_ctx, NULL, &membs,
			CRT_PROTO_OPC(TEST_CORPC_PREFWD_BASE,
				TEST_CORPC_PREFWD_VER, 0), NULL, 0,
			CRT_RPC_FLAG_EXCLUSIVE,
			crt_tree_topo(CRT_TREE_KNOMIAL, 4), &rpc);
		assert(rc == 0);

		rc = crt_req_send(rpc, corpc_response_hdlr, NULL);
		assert(rc == 0);
	}

	/* rank=3 is not sent shutdown sequence */
	if (my_rank == 3)
		g_do_shutdown = 1;

	while (!g_do_shutdown)
		crt_progress(g_main_ctx, 1000, NULL, NULL);

	D_DEBUG(DB_TEST, "Shutting down\n");

	tc_drain_queue(g_main_ctx);

	rc = crt_context_destroy(g_main_ctx, true);
	assert(rc == 0);

	if (my_rank == 0) {
		rc = crt_group_config_remove(NULL);
		assert(rc == 0);
	}

	rc = crt_finalize();
	assert(rc == 0);

	d_log_fini();

	return 0;
}