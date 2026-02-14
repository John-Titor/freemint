#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_linea.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_cpu.h"

#include <stdint.h>
#include <string.h>

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
		sp = mb_cpu_get_usp();
		return (uint16_t *)(uintptr_t)sp;
	}

	return (uint16_t *)(uintptr_t)(sp + sizeof(struct mb_exception_frame));
}

static int mb_super_prepare_return(struct mb_exception_context *ctx,
				       uint32_t sp_after_rte, uint16_t sr_after_rte)
{
	uint32_t frame_size = (uint32_t)sizeof(struct mb_exception_frame);
	uint32_t frame_dst;
	uint32_t phystop = *mb_lm_phystop();
	uint32_t frame_end_limit = phystop + 2u;

	if (sp_after_rte < frame_size)
		return -1;
	frame_dst = sp_after_rte - frame_size;
	if (frame_dst < 0x100u)
		return -1;
	if (frame_dst & 1u)
		return -1;
	if (frame_dst + frame_size > frame_end_limit)
		return -1;

	/* Trap stub writes ctx->frame to ctx->sp before RTE. */
	ctx->sp = frame_dst;
	ctx->frame.sr = sr_after_rte;
	return 0;
}

static int mb_super_stack_reasonable(uint32_t sp_after_rte)
{
	uint32_t membot = *mb_lm_membot();
	uint32_t phystop = *mb_lm_phystop();
	uint32_t frame_size = (uint32_t)sizeof(struct mb_exception_frame);
	uint32_t max_sp = phystop + frame_size;

	if ((sp_after_rte & 1u) != 0)
		return 0;
	if (sp_after_rte < membot + frame_size)
		return 0;
	if (sp_after_rte > max_sp)
		return 0;
	return 1;
}

void mb_trap1_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	uint32_t ret = 0;
	uint32_t usp = 0;

	mb_user_mode = (ctx->frame.sr & 0x2000u) ? 0u : 1u;
	usp = mb_cpu_get_usp();
	mb_cached_user_usp = usp;

	mb_log_printf("trap1: fnum=%04x args=%04x %04x %04x %04x %04x %04x\n",
		      (uint32_t)fnum,
		      (uint32_t)args[1], (uint32_t)args[2], (uint32_t)args[3],
		      (uint32_t)args[4], (uint32_t)args[5], (uint32_t)args[6]);
	mb_log_printf("trap1: sr=%04x s=%u\n",
		      (uint32_t)ctx->frame.sr,
		      (ctx->frame.sr & 0x2000u) ? 1u : 0u);

	if (fnum == 0x20) {
		uint32_t newsp = mb_arg32(args + 1, 0);
		uint16_t sr = ctx->frame.sr;
		uint16_t sbit = (uint16_t)(sr & 0x2000u);
		uint32_t old_ssp = ctx->sp + (uint32_t)sizeof(struct mb_exception_frame);

		mb_log_printf("Super(newsp=%08x) sr=%04x s=%u usp=%08x\n",
			      newsp, sr, sbit ? 1u : 0u, usp);

		if (newsp == 1u) {
			ret = sbit ? 0xffffffffu : 0u;
			ctx->d[0] = ret;
			mb_user_mode = sbit ? 0u : 1u;
			goto out;
		}

		if (newsp == 0u) {
			if (!sbit) {
				if (!mb_super_stack_reasonable(usp))
					mb_panic("Super(0): invalid usp=%08x membot=%08x phystop=%08x",
						 usp, *mb_lm_membot(), *mb_lm_phystop());
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
				uint32_t user_sp = usp;
				/*
				 * Super(value) from supervisor mode sets the supervisor stack
				 * used after the switch back from user mode. Preserve the
			 * current USP so user mode resumes on its existing stack.
			 */
			if (!mb_super_stack_reasonable(user_sp)) {
				user_sp = newsp;
				mb_cpu_set_usp(user_sp);
				usp = user_sp;
			}
			if (mb_super_prepare_return(ctx, newsp, (uint16_t)(sr & 0xdfffu)) < 0)
				mb_panic("Super(%08x): invalid return frame membot=%08x phystop=%08x sp=%08x",
					 newsp, *mb_lm_membot(), *mb_lm_phystop(), ctx->sp);
		}
		ret = usp;
		ctx->d[0] = ret;
		mb_user_mode = 1u;
		goto out;
	}

	ret = (uint32_t)mb_bdos_dispatch(fnum, args + 1);
	ctx->d[0] = ret;
out:
	mb_log_printf("trap1: fnum=%04x ret=%08x\n",
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
	case 0: return "initial SSP";
	case 1: return "initial PC";
	case 2: return "bus error";
	case 3: return "address error";
	case 4: return "illegal instruction";
	case 5: return "zero divide";
	case 6: return "chk";
	case 7: return "trapv";
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

	if (sr & 0x2000u)
		return;
	if (exc_sp & 1u)
		mb_panic("trap%u: odd exc_sp=%08x sr=%04x usp=%08x",
			 trapno, exc_sp, (uint32_t)sr, usp);
	if (usp & 1u)
		mb_panic("trap%u: odd usp=%08x sr=%04x", trapno, usp, (uint32_t)sr);
}

struct mb_frame_info {
	uint16_t fmt_id;
	uint16_t vec;
	uint32_t words;
};

static int mb_decode_frame_info(uint16_t fmt, struct mb_frame_info *info)
{
	uint16_t off = fmt & 0x0fff;
	uint16_t fmt_id = (uint16_t)((fmt >> 12) & 0x000fu);

	info->fmt_id = fmt_id;
	info->vec = 0xffffu;
	info->words = 0;

	if ((off & 3u) != 0 || off > 0x3fcu)
		return 0;

	info->vec = (uint16_t)(off >> 2);

	switch (fmt_id) {
	case 0x0u:
	case 0x1u:
		info->words = 4u;
		break;
	case 0x2u:
		info->words = 6u;
		break;
	case 0x7u:
		info->words = 29u;
		break;
	default:
		info->words = 0u;
		break;
	}

	return 1;
}

void mb_fatal_vector_handler(uint32_t vec, uint32_t sp, uint32_t usp)
{
	struct mb_exception_context ctx;
	struct mb_frame_info fi;
	uint32_t phystop = *mb_lm_phystop();
	uint32_t frame_sp = sp;
	uint32_t pc_swapped;

	memset(&ctx, 0, sizeof(ctx));
	if ((frame_sp & 1u) != 0 || frame_sp + 8u > phystop) {
		uint32_t entry_sp = mb_cached_entry_sp;

		if ((entry_sp & 1u) == 0 && entry_sp + 8u <= phystop) {
			mb_log_printf("fatal vec%u: using entry_sp=%08x (arg sp=%08x)\n",
				      vec, entry_sp, sp);
			frame_sp = entry_sp;
		}
	}

	ctx.sp = frame_sp;
	if ((frame_sp & 1u) == 0 && frame_sp + 8u <= phystop) {
		volatile uint16_t *raw = (volatile uint16_t *)(uintptr_t)frame_sp;

		ctx.frame.sr = raw[0];
		ctx.frame.pc = ((uint32_t)raw[1] << 16) | raw[2];
		ctx.frame.format = raw[3];
		if (mb_decode_frame_info(ctx.frame.format, &fi) && fi.words != 0u) {
			if (frame_sp + (fi.words * 2u) > phystop)
				mb_log_printf("fatal vec%u: frame truncated fmt=%x words=%u sp=%08x phystop=%08x\n",
					      vec, (uint32_t)fi.fmt_id, fi.words, frame_sp, phystop);
			if (fi.vec != (uint16_t)vec)
				mb_log_printf("fatal vec%u: frame vector says %u (fmt=%04x)\n",
					      vec, (uint32_t)fi.vec, (uint32_t)ctx.frame.format);
		}
	}

	mb_cached_user_usp = usp;
	mb_last_ctx_storage = ctx;
	mb_last_ctx = &mb_last_ctx_storage;
	pc_swapped = (ctx.frame.pc << 16) | (ctx.frame.pc >> 16);

	if (vec == 12u && (ctx.frame.pc == 0x03f0c948u || pc_swapped == 0x03f0c948u))
		mb_log_printf("fatal vec12 self-fault near 03f0c948 sp=%08x usp=%08x\n",
			      sp, usp);
	if (vec == 2u &&
	    (pc_swapped >= 0x03f0c900u && pc_swapped < 0x03f0ca40u))
		mb_log_printf("fatal vec2 while entering reserved/line stubs pc(raw)=%08x pc(sw)=%08x\n",
			      ctx.frame.pc, pc_swapped);

	mb_panic("fatal vector %u (%s)", vec, mb_vector_name((uint16_t)vec));
}

void mb_default_exception_handler(struct mb_exception_context *ctx)
{
	uint16_t fmt = ctx->frame.format;
	uint16_t vec = 0xffffu;
	uint32_t sp = ctx->sp;
	uint32_t phystop = *mb_lm_phystop();
	struct mb_frame_info fi;
	int decoded;

	mb_last_ctx_storage = *ctx;
	ctx = &mb_last_ctx_storage;

	decoded = mb_decode_frame_info(fmt, &fi);
	if (decoded)
		vec = fi.vec;

	if (sp >= phystop) {
		vec = 0xffffu;
	} else if ((!decoded || fi.words == 0u) && sp + 8u <= phystop) {
		uint16_t *raw = (uint16_t *)(uintptr_t)sp;
		uint16_t alt = raw[3];

		if (mb_decode_frame_info(alt, &fi)) {
			vec = fi.vec;
			fmt = alt;
		}
	}

	ctx->frame.format = fmt;

	mb_last_ctx = ctx;

	mb_panic("exception %u (%s)", (uint32_t)vec, mb_vector_name(vec));
}
