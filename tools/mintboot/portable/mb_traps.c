#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_trap_helpers.h"

#include <stdint.h>

long mb_rom_gemdos_dispatch(uint16_t fnum, uint16_t *args);
long mb_rom_bios_dispatch(uint16_t fnum, uint16_t *args);
long mb_rom_xbios_dispatch(uint16_t fnum, uint16_t *args);

static inline uint16_t *mb_trap_args(struct mb_exception_context *ctx)
{
	uint32_t sp = ctx->sp;

	if ((ctx->frame.sr & 0x2000u) == 0) {
		__asm__ volatile("move.l %%usp, %0" : "=a"(sp));
		return (uint16_t *)(uintptr_t)sp;
	}

	return (uint16_t *)(uintptr_t)(sp + sizeof(struct mb_exception_frame));
}

void mb_trap1_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];

	if (fnum == 0x20) {
		uint32_t newsp = mb_arg32(args + 1, 0);
		uint16_t *frame = (uint16_t *)(uintptr_t)ctx->sp;
		uint16_t sr = frame[0];
		uint16_t sbit = (uint16_t)(sr & 0x2000u);
		uint32_t usp = 0;

		__asm__ volatile("move.l %%usp, %0" : "=a"(usp));

		mb_log_printf("Super(newsp=%08x) sr=%04x s=%u usp=%08x\r\n",
			      newsp, sr, sbit ? 1u : 0u, usp);

		if (newsp == 1u) {
			ctx->d[0] = sbit ? 0xffffffffu : 0u;
			return;
		}

		if (newsp == 0u) {
			if (!sbit) {
				frame[0] = (uint16_t)(sr | 0x2000u);
			}
			ctx->d[0] = usp;
			return;
		}

		__asm__ volatile("move.l %0, %%usp" : : "a"(newsp));
		frame[0] = (uint16_t)(sr & 0xdfffu);
		ctx->d[0] = usp;
		return;
	}

	ctx->d[0] = (uint32_t)mb_rom_gemdos_dispatch(fnum, args + 1);
}

void mb_trap2_handler(struct mb_exception_context *ctx)
{
	uint16_t fnum = (uint16_t)ctx->d[0];

	if (fnum == 0) {
		ctx->d[0] = (uint32_t)mb_rom_pterm0();
		return;
	}
	ctx->d[0] = (uint32_t)MB_ERR_INVFN;
}

void mb_trap13_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	ctx->d[0] = (uint32_t)mb_rom_bios_dispatch(fnum, args + 1);
}

void mb_trap14_handler(struct mb_exception_context *ctx)
{
	uint16_t *args = mb_trap_args(ctx);
	uint16_t fnum = args[0];
	ctx->d[0] = (uint32_t)mb_rom_xbios_dispatch(fnum, args + 1);
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

struct mb_exception_context *mb_last_exception_context(void)
{
	return mb_last_ctx;
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
