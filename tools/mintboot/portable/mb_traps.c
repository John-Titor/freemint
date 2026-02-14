#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_linea.h"
#include "mintboot/mb_trap_helpers.h"

#include <stdint.h>

long mb_bdos_dispatch(uint16_t fnum, uint16_t *args);
long mb_bios_dispatch(uint16_t fnum, uint16_t *args);
long mb_xbios_dispatch(uint16_t fnum, uint16_t *args);

volatile uint16_t mb_user_mode;
static uint32_t mb_cached_user_usp;
volatile uint32_t mb_cached_entry_usp;
volatile uint32_t mb_cached_entry_sp;

static inline uint16_t *mb_trap_args(struct mb_exception_context *ctx)
{
	uint32_t sp = ctx->sp;

	if ((ctx->frame.sr & 0x2000u) == 0) {
		__asm__ volatile("move.l %%usp, %0" : "=a"(sp));
		return (uint16_t *)(uintptr_t)sp;
	}

	return (uint16_t *)(uintptr_t)(sp + sizeof(struct mb_exception_frame));
}

static int mb_super_prepare_return(struct mb_exception_context *ctx,
				       uint32_t sp_after_rte, uint16_t sr_after_rte)
{
	uint32_t frame_size = (uint32_t)sizeof(struct mb_exception_frame);
	uint32_t frame_dst;
	struct mb_exception_frame *src;
	struct mb_exception_frame *dst;

	if (sp_after_rte < frame_size)
		return -1;
	frame_dst = sp_after_rte - frame_size;
	if (frame_dst < 0x100u)
		return -1;
	if (frame_dst & 1u)
		return -1;

	src = (struct mb_exception_frame *)(uintptr_t)ctx->sp;
	dst = (struct mb_exception_frame *)(uintptr_t)frame_dst;
	*dst = *src;
	dst->sr = sr_after_rte;
	ctx->sp = frame_dst;
	return 0;
}

void mb_trap1_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	uint32_t ret = 0;
	uint32_t usp = 0;

	mb_user_mode = (ctx->frame.sr & 0x2000u) ? 0u : 1u;
	__asm__ volatile("move.l %%usp, %0" : "=a"(usp));
	mb_cached_user_usp = usp;

	mb_log_printf("trap1: fnum=%04x args=%04x %04x %04x %04x %04x %04x\r\n",
		      (uint32_t)fnum,
		      (uint32_t)args[1], (uint32_t)args[2], (uint32_t)args[3],
		      (uint32_t)args[4], (uint32_t)args[5], (uint32_t)args[6]);

	if (fnum == 0x20) {
		uint32_t newsp = mb_arg32(args + 1, 0);
		uint16_t sr = ctx->frame.sr;
		uint16_t sbit = (uint16_t)(sr & 0x2000u);
		uint32_t old_ssp = ctx->sp + (uint32_t)sizeof(struct mb_exception_frame);
		uint32_t sup_sp_after = old_ssp;

		mb_log_printf("Super(newsp=%08x) sr=%04x s=%u usp=%08x\r\n",
			      newsp, sr, sbit ? 1u : 0u, usp);

		if (newsp == 1u) {
			ret = sbit ? 0xffffffffu : 0u;
			ctx->d[0] = ret;
			mb_user_mode = sbit ? 0u : 1u;
			goto out;
		}

		if (newsp == 0u) {
			if (!sbit) {
				if (mb_super_prepare_return(ctx, usp,
							    (uint16_t)(sr | 0x2000u)) < 0)
					mb_panic("Super(0): invalid usp=%08x", usp);
			}
			ret = old_ssp;
			ctx->d[0] = ret;
			mb_user_mode = 0u;
			goto out;
		}

		if (sbit) {
			/* Return to user mode at this callsite using current supervisor stack. */
			__asm__ volatile("move.l %0, %%usp" : : "a"(sup_sp_after));
			if (mb_super_prepare_return(ctx, newsp, (uint16_t)(sr & 0xdfffu)) < 0)
				mb_panic("Super(%08x): invalid ssp", newsp);
		}
		ret = usp;
		ctx->d[0] = ret;
		mb_user_mode = 1u;
		goto out;
	}

	ret = (uint32_t)mb_bdos_dispatch(fnum, args + 1);
	ctx->d[0] = ret;
out:
	mb_log_printf("trap1: fnum=%04x ret=%08x\r\n",
		      (uint32_t)fnum, ctx->d[0]);
}

void mb_trap2_handler(struct mb_exception_context *ctx)
{
	uint16_t fnum = (uint16_t)ctx->d[0];

	mb_user_mode = (ctx->frame.sr & 0x2000u) ? 0u : 1u;
	if (fnum == 0) {
		ctx->d[0] = (uint32_t)mb_bdos_pterm0();
		return;
	}
	ctx->d[0] = (uint32_t)MB_ERR_INVFN;
}

void mb_trap13_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	mb_user_mode = (ctx->frame.sr & 0x2000u) ? 0u : 1u;
	ctx->d[0] = (uint32_t)mb_bios_dispatch(fnum, args + 1);
}

void mb_trap14_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	mb_user_mode = (ctx->frame.sr & 0x2000u) ? 0u : 1u;
	ctx->d[0] = (uint32_t)mb_xbios_dispatch(fnum, args + 1);
}

void mb_spurious_irq_handler(void)
{
	/* TODO: spurious interrupt handler. */
}

void __attribute__((weak)) mb_autovec_level1_handler(void)
{
	mb_panic("autovec level1");
}

void __attribute__((weak)) mb_autovec_level2_handler(void)
{
	mb_panic("autovec level2");
}

void __attribute__((weak)) mb_autovec_level3_handler(void)
{
	mb_panic("autovec level3");
}

void __attribute__((weak)) mb_autovec_level4_handler(void)
{
	mb_panic("autovec level4");
}

void __attribute__((weak)) mb_autovec_level5_handler(void)
{
	mb_panic("autovec level5");
}

void __attribute__((weak)) mb_autovec_level6_handler(void)
{
	mb_panic("autovec level6");
}

void __attribute__((weak)) mb_autovec_level7_handler(void)
{
	mb_panic("autovec level7");
}

void mb_trace_exception_handler(struct mb_exception_context *ctx)
{
	volatile uint16_t *frame_sr = (uint16_t *)(uintptr_t)ctx->sp;

	*frame_sr = (uint16_t)(*frame_sr & 0x3fffu);
}

void mb_linea_exception_handler(struct mb_exception_context *ctx)
{
	uint32_t pc = ctx->frame.pc;
	volatile const uint16_t *op_at_pc = (volatile const uint16_t *)(uintptr_t)pc;
	uint16_t op = *op_at_pc;

	if (op != 0xa000u && pc >= 2u) {
		volatile const uint16_t *op_prev =
			(volatile const uint16_t *)(uintptr_t)(pc - 2u);
		uint16_t prev = *op_prev;

		if (prev == 0xa000u)
			op = prev;
	}

	if (op == 0xa000u) {
		uint32_t ptr = mb_linea_planes_ptr();

		ctx->d[0] = ptr;
		ctx->a[0] = ptr;
		if (*op_at_pc == 0xa000u)
			ctx->frame.pc = pc + 2u;
		return;
	}

	mb_panic("lineA: unsupported opcode %04x at pc=%08x",
		 (uint32_t)op, pc);
}


static const char *mb_vector_name(uint16_t vec)
{
	switch (vec) {
	case 0: return "reset";
	case 1: return "bus error";
	case 2: return "address error";
	case 3: return "illegal instruction";
	case 4: return "zero divide";
	case 5: return "chk";
	case 6: return "trapv";
	case 7: return "reserved 7";
	case 8: return "privilege violation";
	case 9: return "trace";
	case 10: return "line 1010";
	case 11: return "line 1111";
	case 12: return "reserved 12";
	case 13: return "reserved 13";
	case 14: return "reserved 14";
	case 15: return "reserved 15";
	case 24: return "spurious";
	case 25: return "irq1";
	case 26: return "irq2";
	case 27: return "irq3";
	case 28: return "irq4";
	case 29: return "irq5";
	case 30: return "irq6";
	case 31: return "irq7";
	case 32: return "trap0";
	case 33: return "trap1";
	case 34: return "trap2";
	case 35: return "trap3";
	case 36: return "trap4";
	case 37: return "trap5";
	case 38: return "trap6";
	case 39: return "trap7";
	case 40: return "trap8";
	case 41: return "trap9";
	case 42: return "trap10";
	case 43: return "trap11";
	case 44: return "trap12";
	case 45: return "trap13";
	case 46: return "trap14";
	case 47: return "trap15";
	default: return "unknown";
	}
}

static struct mb_exception_context mb_last_ctx_storage;
static struct mb_exception_context *mb_last_ctx;
void mb_trap_return_sanity(uint32_t trapno, uint32_t exc_sp, uint32_t usp,
			       uint32_t frame_sr);

struct mb_exception_context *mb_last_exception_context(void)
{
	return mb_last_ctx;
}

uint32_t mb_last_user_usp(void)
{
	return mb_cached_user_usp;
}

uint32_t mb_last_entry_usp(void)
{
	return mb_cached_entry_usp;
}

uint32_t mb_last_entry_sp(void)
{
	return mb_cached_entry_sp;
}

void mb_trap_return_sanity(uint32_t trapno, uint32_t exc_sp, uint32_t usp,
			       uint32_t frame_sr)
{
	uint16_t sr = (uint16_t)frame_sr;
	uint32_t membot = *mb_lm_membot();
	uint32_t phystop = *mb_lm_phystop();

	if (exc_sp < membot || exc_sp + sizeof(struct mb_exception_frame) > phystop)
		mb_panic("trap%u: bad exc_sp=%08x sr=%04x membot=%08x phystop=%08x",
			 trapno, exc_sp, (uint32_t)sr, membot, phystop);

	if (sr & 0x2000u)
		return;
	if (exc_sp & 1u)
		mb_panic("trap%u: odd exc_sp=%08x sr=%04x usp=%08x",
			 trapno, exc_sp, (uint32_t)sr, usp);
	if (usp & 1u)
		mb_panic("trap%u: odd usp=%08x sr=%04x", trapno, usp, (uint32_t)sr);
}

static int mb_decode_vector(uint16_t fmt, uint16_t *vec)
{
	uint16_t off = fmt & 0x0fff;

	if ((off & 3u) != 0 || off > 0x3fcu)
		return 0;
	*vec = (uint16_t)(off >> 2);
	return 1;
}

void mb_default_exception_handler(struct mb_exception_context *ctx)
{
	uint16_t fmt = ctx->frame.format;
	uint16_t vec = 0xffffu;
	uint32_t sp = ctx->sp;
	uint32_t phystop = *mb_lm_phystop();

	mb_last_ctx_storage = *ctx;
	ctx = &mb_last_ctx_storage;

	if (sp >= phystop) {
		vec = 0xffffu;
	} else if (!mb_decode_vector(fmt, &vec) && sp + 8 <= phystop) {
		uint16_t *raw = (uint16_t *)(uintptr_t)sp;
		uint16_t alt = raw[3];

		if (mb_decode_vector(alt, &vec))
			fmt = alt;
	}

	ctx->frame.format = fmt;

	mb_last_ctx = ctx;

	mb_panic("exception %u (%s)", (uint32_t)vec, mb_vector_name(vec));
}
