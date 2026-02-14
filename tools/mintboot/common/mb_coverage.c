#include "mintboot/mb_common.h"

#include <stdint.h>

#if defined(MB_ENABLE_COVERAGE) && MB_ENABLE_COVERAGE

#include <gcov.h>

extern const struct gcov_info *const __gcov_info_start[];
extern const struct gcov_info *const __gcov_info_end[];

struct mb_cov_ctx {
	uint8_t alloc_buf[32768];
	unsigned alloc_used;
};

static void mb_cov_dump_hex(const void *data, unsigned len, void *arg)
{
	const uint8_t *p = (const uint8_t *)data;
	unsigned i;
	(void)arg;

	for (i = 0; i < len; ) {
		unsigned chunk = len - i;
		unsigned j;
		static const char hex[] = "0123456789abcdef";
		char line[8 + (24 * 2) + 2 + 1];
		unsigned out = 0;

		if (chunk > 24u)
			chunk = 24u;
		line[out++] = 'M';
		line[out++] = 'B';
		line[out++] = '-';
		line[out++] = 'C';
		line[out++] = 'O';
		line[out++] = 'V';
		line[out++] = ':';
		for (j = 0; j < chunk; j++) {
			uint8_t b = p[i + j];
			line[out++] = hex[(b >> 4) & 0x0f];
			line[out++] = hex[b & 0x0f];
		}
		line[out++] = '\r';
		line[out++] = '\n';
		line[out] = '\0';
		mb_log_puts(line);
		i += chunk;
	}
}

static void mb_cov_filename(const char *filename, void *arg)
{
	__gcov_filename_to_gcfn(filename, mb_cov_dump_hex, arg);
}

static void *mb_cov_alloc(unsigned size, void *arg)
{
	struct mb_cov_ctx *ctx = (struct mb_cov_ctx *)arg;
	unsigned aligned = (size + 3u) & ~3u;
	void *p;

	if (aligned > sizeof(ctx->alloc_buf) ||
	    ctx->alloc_used + aligned > sizeof(ctx->alloc_buf))
		mb_panic("coverage alloc overflow size=%u used=%u", size,
			 ctx->alloc_used);

	p = &ctx->alloc_buf[ctx->alloc_used];
	ctx->alloc_used += aligned;
	return p;
}

void mb_coverage_dump(void)
{
	const struct gcov_info *const *info;

	if (&__gcov_info_start[0] >= &__gcov_info_end[0])
		return;

	mb_log_puts("MB-COV-BEGIN\n");
	for (info = __gcov_info_start; info < __gcov_info_end; info++) {
		struct mb_cov_ctx ctx;

		ctx.alloc_used = 0;
		if (*info) {
			__gcov_info_to_gcda(*info,
					    mb_cov_filename,
					    mb_cov_dump_hex,
					    mb_cov_alloc,
					    &ctx);
		}
	}
	mb_log_puts("MB-COV-END\n");
}

void __gcov_merge_add(void *counters, unsigned int n);
void __gcov_merge_add(void *counters, unsigned int n)
{
	(void)counters;
	(void)n;
}

void abort(void) __attribute__((noreturn));
void abort(void)
{
	mb_panic("abort");
	for (;;)
		;
}

#else

void mb_coverage_dump(void)
{
}

#endif
