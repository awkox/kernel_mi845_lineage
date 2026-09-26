// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/zstd.h>

#include "backend_zstd.h"

/*
 * ZSTD_CLEVEL_DEFAULT does not exist in this kernel's zstd version (1.3.x),
 * it is 3 and is what crypto/zstd.c hardcodes as well.
 */
#define ZSTD_DEF_LEVEL	3

struct zstd_ctx {
	ZSTD_CCtx *cctx;
	ZSTD_DCtx *dctx;
	void *cctx_mem;
	void *dctx_mem;
};

struct zstd_params {
	ZSTD_parameters cprm;
};

static void zstd_release_params(struct zcomp_params *params)
{
	struct zstd_params *zp = params->drv_data;

	params->drv_data = NULL;
	if (!zp)
		return;

	kfree(zp);
}

static int zstd_setup_params(struct zcomp_params *params)
{
	struct zstd_params *zp;

	zp = kzalloc(sizeof(*zp), GFP_KERNEL);
	if (!zp)
		return -ENOMEM;

	params->drv_data = zp;
	if (params->level == ZCOMP_PARAM_NOT_SET)
		params->level = ZSTD_DEF_LEVEL;

	zp->cprm = ZSTD_getParams(params->level, PAGE_SIZE, 0);

	return 0;
}

static void zstd_destroy(struct zcomp_ctx *ctx)
{
	struct zstd_ctx *zctx = ctx->context;

	if (!zctx)
		return;

	vfree(zctx->cctx_mem);
	vfree(zctx->dctx_mem);
	kfree(zctx);
}

static int zstd_create(struct zcomp_params *params, struct zcomp_ctx *ctx)
{
	struct zstd_ctx *zctx;
	ZSTD_parameters prm;
	size_t sz;

	zctx = kzalloc(sizeof(*zctx), GFP_KERNEL);
	if (!zctx)
		return -ENOMEM;

	ctx->context = zctx;
	prm = ZSTD_getParams(params->level, PAGE_SIZE, 0);
	sz = ZSTD_CCtxWorkspaceBound(prm.cParams);
	zctx->cctx_mem = vzalloc(sz);
	if (!zctx->cctx_mem)
		goto error;

	zctx->cctx = ZSTD_initCCtx(zctx->cctx_mem, sz);
	if (!zctx->cctx)
		goto error;

	sz = ZSTD_DCtxWorkspaceBound();
	zctx->dctx_mem = vzalloc(sz);
	if (!zctx->dctx_mem)
		goto error;

	zctx->dctx = ZSTD_initDCtx(zctx->dctx_mem, sz);
	if (!zctx->dctx)
		goto error;

	return 0;

error:
	zstd_destroy(ctx);
	return -EINVAL;
}

static int zstd_compress(struct zcomp_params *params, struct zcomp_ctx *ctx,
			 struct zcomp_req *req)
{
	struct zstd_params *zp = params->drv_data;
	struct zstd_ctx *zctx = ctx->context;
	size_t ret;

	ret = ZSTD_compressCCtx(zctx->cctx, req->dst, req->dst_len,
				req->src, req->src_len, zp->cprm);
	if (ZSTD_isError(ret))
		return -EINVAL;
	req->dst_len = ret;
	return 0;
}

static int zstd_decompress(struct zcomp_params *params, struct zcomp_ctx *ctx,
			   struct zcomp_req *req)
{
	struct zstd_ctx *zctx = ctx->context;
	size_t ret;

	ret = ZSTD_decompressDCtx(zctx->dctx, req->dst, req->dst_len,
				   req->src, req->src_len);
	if (ZSTD_isError(ret))
		return -EINVAL;
	return 0;
}

const struct zcomp_ops backend_zstd = {
	.compress	= zstd_compress,
	.decompress	= zstd_decompress,
	.create_ctx	= zstd_create,
	.destroy_ctx	= zstd_destroy,
	.setup_params	= zstd_setup_params,
	.release_params	= zstd_release_params,
	.name		= "zstd",
};
