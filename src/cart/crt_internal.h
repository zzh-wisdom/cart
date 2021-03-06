/* Copyright (C) 2016-2019 Intel Corporation
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
 * \file src/cart/crt_internal.h
 * 
 * This file is part of CaRT. It it the common header file which be included by
 * all other .c files of CaRT.
 * 
 * 此文件是 CART 的一部分。它是其他.c文件include的公用头文件。
 */

#ifndef __CRT_INTERNAL_H__
#define __CRT_INTERNAL_H__

#include "crt_debug.h"

#include <gurt/common.h>
#include <gurt/fault_inject.h>
#include <cart/api.h>

#include "crt_hg.h"
#include "crt_internal_types.h"
#include "crt_internal_fns.h"
#include "crt_rpc.h"
#include "crt_group.h"
#include "crt_tree.h"
#include "crt_self_test.h"
#include "crt_ctl.h"
#include "crt_swim.h"

/**
 * @brief RPC_TRACE RPC跟踪
 * 
 * A wrapper around D_TRACE_DEBUG that ensures the ptr option is a RPC
 * 
 * 围绕D_TRACE_DEBUG的包装，需要确保ptr选项为RPC
 * 
 */
#define RPC_TRACE(mask, rpc, fmt, ...)					\
	do {								\
		D_TRACE_DEBUG(mask, (rpc),				\
			" [opc=0x%x xid=0x%x rank:tag=%d:%d] " fmt,	\
			(rpc)->crp_pub.cr_opc,				\
			(rpc)->crp_req_hdr.cch_xid,			\
			(rpc)->crp_pub.cr_ep.ep_rank,			\
			(rpc)->crp_pub.cr_ep.ep_tag,			\
			## __VA_ARGS__);				\
	} while (0)

/* Log an error with a RPC descriptor */
/**
 * @brief RPC_ERROR RPC报错
 * 
 * 使用 RPC 描述符记录错误
 * 
 */
#define RPC_ERROR(rpc, fmt, ...)					\
	do {								\
		D_TRACE_ERROR((rpc),					\
			" [opc=0x%x xid=0x%x rank:tag=%d:%d] " fmt,	\
			(rpc)->crp_pub.cr_opc,				\
			(rpc)->crp_req_hdr.cch_xid,			\
			(rpc)->crp_pub.cr_ep.ep_rank,			\
			(rpc)->crp_pub.cr_ep.ep_tag,			\
			## __VA_ARGS__);				\
	} while (0)

#endif /* __CRT_INTERNAL_H__ */
