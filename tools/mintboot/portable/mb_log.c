#include "mintboot/mb_board.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_kernel.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_cpu.h"

#include <stdarg.h>

extern uint8_t _mb_image_end[] __attribute__((weak));
extern volatile uint16_t mb_user_mode;

static int mb_pc_in_known_region(uint32_t pc, uint32_t kbase, uint32_t kend,
				 uint32_t mb_end)
{
	if (kbase && kend && pc >= kbase && pc < kend)
		return 1;
	if (mb_end && pc < mb_end)
		return 1;
	return 0;
}

static uint32_t mb_wordswap32(uint32_t v)
{
	return (v << 16) | (v >> 16);
}

struct mb_frame_decode {
	uint16_t fmt_id;
	uint16_t vec;
	uint16_t vec_off;
	uint32_t words;
	uint32_t fault_addr_word;
	int has_fault_addr;
	const char *name;
};

static struct mb_frame_decode mb_decode_exception_frame(uint16_t fmt_word)
{
	struct mb_frame_decode d;

	d.fmt_id = (uint16_t)((fmt_word >> 12) & 0x0fu);
	d.vec_off = (uint16_t)(fmt_word & 0x0fffu);
	d.vec = 0xffffu;
	d.words = 0;
	d.fault_addr_word = 0;
	d.has_fault_addr = 0;
	d.name = "unknown";

	if ((d.vec_off & 3u) == 0 && d.vec_off <= 0x3fcu)
		d.vec = (uint16_t)(d.vec_off >> 2);

	switch (d.fmt_id) {
	case 0x0u:
		d.name = "short";
		d.words = 4;
		break;
	case 0x1u:
		d.name = "throwaway";
		d.words = 4;
		break;
	case 0x2u:
		d.name = "format2";
		d.words = 6;
		break;
	case 0x7u:
		d.name = "68040 access";
		d.words = 29;
		d.has_fault_addr = 1;
		d.fault_addr_word = 4;
		break;
	default:
		break;
	}

	return d;
}

static void mb_log_putc(int ch)
{
	if (ch == '\n')
		mb_log_putc('\r');

	if (mb_user_mode)
		Bconout(2, ch);
	else
		mb_board_console_putc(ch);
}

static void mb_log_hex32_width(uint32_t value, int width, int pad_zero)
{
	static const char hex[] = "0123456789abcdef";
	char buf[8];
	int i;
	int len = 0;
	char pad = pad_zero ? '0' : ' ';

	for (i = 0; i < 8; i++) {
		buf[i] = hex[(value >> ((7 - i) * 4)) & 0xf];
		if (!len && buf[i] != '0')
			len = 8 - i;
	}
	if (!len)
		len = 1;

	for (i = len; i < width; i++)
		mb_log_putc(pad);
	for (i = 8 - len; i < 8; i++)
		mb_log_putc(buf[i]);
}

static void mb_log_hex64_width(uint64_t value, int width, int pad_zero)
{
	static const char hex[] = "0123456789abcdef";
	char buf[16];
	int i;
	int len = 0;
	char pad = pad_zero ? '0' : ' ';

	for (i = 0; i < 16; i++) {
		buf[i] = hex[(value >> ((15 - i) * 4)) & 0xf];
		if (!len && buf[i] != '0')
			len = 16 - i;
	}
	if (!len)
		len = 1;

	for (i = len; i < width; i++)
		mb_log_putc(pad);
	for (i = 16 - len; i < 16; i++)
		mb_log_putc(buf[i]);
}

void mb_log_puts(const char *s)
{
	for (; *s; s++)
		mb_log_putc(*s);
}

void mb_log_hex32(uint32_t value)
{
	static const char hex[] = "0123456789abcdef";
	int shift;

	mb_log_puts("0x");
	for (shift = 28; shift >= 0; shift -= 4)
		mb_log_putc(hex[(value >> shift) & 0xf]);
}

void mb_log_hex64(uint64_t value)
{
	static const char hex[] = "0123456789abcdef";
	int shift;

	mb_log_puts("0x");
	for (shift = 60; shift >= 0; shift -= 4)
		mb_log_putc(hex[(value >> shift) & 0xf]);
}

void mb_log_u32(uint32_t value)
{
	char buf[10];
	int i = 0;

	if (value == 0) {
		mb_log_putc('0');
		return;
	}

	while (value && i < (int)sizeof(buf)) {
		buf[i++] = '0' + (value % 10);
		value /= 10;
	}

	while (i--)
		mb_log_putc(buf[i]);
}

void mb_log_i32(int32_t value)
{
	if (value < 0) {
		mb_log_putc('-');
		mb_log_u32((uint32_t)(-value));
		return;
	}

	mb_log_u32((uint32_t)value);
}

void mb_log_vprintf(const char *fmt, va_list ap)
{
	const char *s;
	int width;
	int pad_zero;

	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			mb_log_putc(*fmt);
			continue;
		}

		fmt++;
		if (*fmt == '%') {
			mb_log_putc('%');
			continue;
		}

		pad_zero = 0;
		width = 0;
		if (*fmt == '0') {
			pad_zero = 1;
			fmt++;
		}
		while (*fmt >= '0' && *fmt <= '9') {
			width = (width * 10) + (*fmt - '0');
			fmt++;
		}

		if (*fmt == 'l' && *(fmt + 1) == 'l') {
			fmt += 2;
			if (*fmt == 'x') {
				mb_log_hex64_width(va_arg(ap, uint64_t), width, pad_zero);
				continue;
			}
			if (*fmt == 'u') {
				mb_log_u32((uint32_t)va_arg(ap, uint64_t));
				continue;
			}
			if (*fmt == 'd') {
				mb_log_i32((int32_t)va_arg(ap, int64_t));
				continue;
			}
			mb_log_putc('?');
			continue;
		}

		switch (*fmt) {
		case 's':
			s = va_arg(ap, const char *);
			if (!s)
				s = "(null)";
			mb_log_puts(s);
			break;
		case 'c':
			mb_log_putc(va_arg(ap, int));
			break;
		case 'x':
			mb_log_hex32_width(va_arg(ap, uint32_t), width, pad_zero);
			break;
		case 'u':
			mb_log_u32(va_arg(ap, uint32_t));
			break;
		case 'd':
			mb_log_i32(va_arg(ap, int32_t));
			break;
		default:
			mb_log_putc('?');
			break;
		}
	}
}

void mb_log_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	mb_log_vprintf(fmt, ap);
	va_end(ap);
}

void mb_panic(const char *fmt, ...)
{
	va_list ap;
	struct mb_exception_context *ctx;
	uint32_t usp = 0;
	uint32_t usp_cached = 0;
	uint32_t usp_dump = 0;
	uint32_t entry_usp = 0;
	uint32_t entry_sp = 0;
	uint32_t ssp = 0;
	uint32_t vbr = 0;
	ssp = mb_cpu_get_sp();
	if (!mb_user_mode) {
		usp = mb_cpu_get_usp();
		vbr = mb_cpu_get_vbr();
	}
	usp_cached = mb_last_user_usp();
	entry_usp = mb_last_entry_usp();
	entry_sp = mb_last_entry_sp();
	usp_dump = usp ? usp : usp_cached;

	mb_log_puts("\nmintboot panic: ");
	va_start(ap, fmt);
	mb_log_vprintf(fmt, ap);
	va_end(ap);
	mb_log_puts("\n");

	ctx = mb_last_exception_context();
	mb_log_printf("  USP=%08x USP(cached)=%08x SSP=%08x\n",
		      usp, usp_cached, ssp);
	mb_log_printf("  Entry USP=%08x Entry SP=%08x\n",
		      entry_usp, entry_sp);
	if (ctx) {
		uint32_t pc_raw = ctx->frame.pc;
		uint32_t pc = pc_raw;
		uint32_t kbase = 0;
		uint32_t kend = 0;
		uint32_t mb_end = 0;
		const char *where = "elsewhere";
		uint16_t *raw = (uint16_t *)(uintptr_t)ctx->sp;
		uint32_t phystop = *mb_lm_phystop();
		uint32_t sp = ctx->sp;
		struct mb_frame_decode frame = mb_decode_exception_frame(ctx->frame.format);
		uint16_t vec = frame.vec;
		uint32_t vec_entry = 0;
		int i;

		mb_portable_kernel_bounds(&kbase, &kend);
		if (_mb_image_end)
			mb_end = (uint32_t)(uintptr_t)_mb_image_end;
		{
			uint32_t pc_swapped = mb_wordswap32(pc_raw);
			int raw_known = mb_pc_in_known_region(pc_raw, kbase, kend, mb_end);
			int swapped_known = mb_pc_in_known_region(pc_swapped, kbase, kend, mb_end);

			if (!raw_known && swapped_known)
				pc = pc_swapped;
		}
		if (kbase && kend && pc >= kbase && pc < kend) {
			where = "kernel";
		} else if (pc < mb_end) {
			where = "mintboot";
		}
		mb_log_printf("  SR=%04x PC=%08x FMT=%04x\n",
			      (uint32_t)ctx->frame.sr,
			      pc,
			      (uint32_t)ctx->frame.format);
		if (pc != pc_raw)
			mb_log_printf("  PC(raw)=%08x (wordswapped)\n", pc_raw);
		mb_log_printf("  Frame decode: fmt=%x (%s) vec_off=%03x words=%u\n",
			      (uint32_t)frame.fmt_id, frame.name,
			      (uint32_t)frame.vec_off, frame.words);
		if (frame.has_fault_addr &&
		    sp + 2u * (frame.fault_addr_word + 2u) <= phystop) {
			uint32_t fault_addr = ((uint32_t)raw[frame.fault_addr_word] << 16) |
					      (uint32_t)raw[frame.fault_addr_word + 1u];
			mb_log_printf("  Fault addr=%08x\n", fault_addr);
		}
		mb_log_printf("  VBR=%08x VECTOR=%u\n", vbr, (uint32_t)vec);
		if (vec != 0xffffu) {
			uint32_t *vt = (uint32_t *)(uintptr_t)(vbr + ((uint32_t)vec << 2));
			vec_entry = *vt;
			mb_log_printf("  VEC[%u]=%08x\n", (uint32_t)vec, vec_entry);
		}
		mb_log_printf("  PC region: %s\n", where);
		mb_log_printf("  PHYTOP=%08x SP=%08x\n", phystop, sp);
		mb_log_printf("  SP region: %s\n",
			      (sp < phystop) ? "st-ram" : "invalid");
		if (pc < phystop) {
			uint32_t pc_start = (pc >= 8u) ? (pc - 8u) : 0u;
			uint32_t pc_end = pc + 8u;
			uint16_t *code;
			uint32_t count;

			if (pc_end > phystop)
				pc_end = phystop;
			count = (pc_end - pc_start) / 2u;
			code = (uint16_t *)(uintptr_t)pc_start;

			mb_log_printf("  PC words: pc=%08x start=%08x end=%08x\n",
				      pc, pc_start, pc_end);
			mb_log_puts("  Code:");
			for (i = 0; i < (int)count; i++) {
				uint32_t addr = pc_start + ((uint32_t)i * 2u);

				if ((i % 6) == 0)
					mb_log_puts("\n   ");
				if (addr == pc)
					mb_log_puts(">");
				else
					mb_log_puts(" ");
				mb_log_printf("%04x", (uint32_t)code[i]);
			}
			mb_log_puts("\n");
		}
		if (sp < phystop) {
			uint32_t avail_words = (phystop - sp) / 2u;
			uint32_t want_words = frame.words ? frame.words : 12u;
			uint32_t dump_words = (avail_words < want_words) ? avail_words : want_words;

			mb_log_puts("  Frame words:");
			for (i = 0; i < (int)dump_words; i++) {
				if ((i % 6) == 0)
					mb_log_puts("\n   ");
				mb_log_printf(" %04x", (uint32_t)raw[i]);
			}
			if (dump_words < want_words)
				mb_log_puts(" (partial)");
			mb_log_puts("\n");
		} else {
			mb_log_puts("  Frame words: unavailable\n");
		}
		if (sp < phystop) {
			uint32_t avail_words = (phystop - sp) / 2u;
			uint32_t dump_words = (avail_words < 48u) ? avail_words : 48u;

			mb_log_puts("  SSP words:");
			for (i = 0; i < (int)dump_words; i++) {
				if ((i % 8) == 0)
					mb_log_puts("\n   ");
				mb_log_printf(" %04x", (uint32_t)raw[i]);
			}
			if (dump_words < 48u)
				mb_log_puts(" (partial)");
			mb_log_puts("\n");
		} else {
			mb_log_puts("  SSP words: unavailable\n");
		}
		if (frame.words && sp + 2u * (frame.words + 16u) <= phystop) {
			uint16_t *post = raw + frame.words;
			mb_log_puts("  Post-frame words:");
			for (i = 0; i < 16; i++) {
				if ((i % 8) == 0)
					mb_log_puts("\n   ");
				mb_log_printf(" %04x", (uint32_t)post[i]);
			}
			mb_log_puts("\n");
			if (sp + 2u * (frame.words + 32u) <= phystop) {
				uint32_t *pdw = (uint32_t *)(uintptr_t)post;
				int printed = 0;

				mb_log_puts("  Post-frame dwords (kernel range):");
				for (i = 0; i < 16; i++) {
					uint32_t val = pdw[i];
					if (kbase && kend && val >= kbase && val < kend) {
						mb_log_printf(" [%u]=%08x", i, val);
						printed = 1;
					}
				}
				if (!printed)
					mb_log_puts(" none");
				mb_log_puts("\n");
			}
		}
		if (sp + 4u * 24u <= phystop) {
			uint32_t *dw = (uint32_t *)(uintptr_t)sp;
			int printed = 0;

			mb_log_puts("  SSP dwords (kernel range):");
			for (i = 0; i < 24; i++) {
				uint32_t val = dw[i];
				if (kbase && kend && val >= kbase && val < kend) {
					mb_log_printf(" [%u]=%08x", i, val);
					printed = 1;
				}
			}
			if (!printed)
				mb_log_puts(" none");
			mb_log_puts("\n");
		}
		if (usp_dump && usp_dump < phystop) {
			uint32_t start = (usp_dump >= 2u * 16u) ? (usp_dump - 2u * 16u) : 0u;
			uint32_t end = usp_dump + 2u * 16u;
			uint32_t count;
			uint16_t *u;

			if (end > phystop)
				end = phystop;
			if (end > start + 1u) {
				count = (end - start) / 2u;
				u = (uint16_t *)(uintptr_t)start;
				mb_log_printf("  USP window (%s): USP=%08x start=%08x end=%08x\n",
					      usp ? "live" : "cached",
					      usp_dump, start, end);
				mb_log_puts("  USP words (around):");
				for (i = 0; i < (int)count; i++) {
					if ((i % 8) == 0)
						mb_log_puts("\n   ");
					if (start + (uint32_t)(i * 2) == usp_dump)
						mb_log_puts(">");
					else
						mb_log_puts(" ");
					mb_log_printf("%04x", (uint32_t)u[i]);
				}
				mb_log_puts("\n");
			}
		}
		mb_log_printf("  D0=%08x (%d)\n", ctx->d[0], (int32_t)ctx->d[0]);
		mb_log_printf("  D1=%08x (%d)\n", ctx->d[1], (int32_t)ctx->d[1]);
		mb_log_printf("  D2=%08x (%d)\n", ctx->d[2], (int32_t)ctx->d[2]);
		mb_log_printf("  D3=%08x (%d)\n", ctx->d[3], (int32_t)ctx->d[3]);
		mb_log_printf("  D4=%08x (%d)\n", ctx->d[4], (int32_t)ctx->d[4]);
		mb_log_printf("  D5=%08x (%d)\n", ctx->d[5], (int32_t)ctx->d[5]);
		mb_log_printf("  D6=%08x (%d)\n", ctx->d[6], (int32_t)ctx->d[6]);
		mb_log_printf("  D7=%08x (%d)\n", ctx->d[7], (int32_t)ctx->d[7]);
		mb_log_printf("  A0=%08x (%u)\n", ctx->a[0], ctx->a[0]);
		mb_log_printf("  A1=%08x (%u)\n", ctx->a[1], ctx->a[1]);
		mb_log_printf("  A2=%08x (%u)\n", ctx->a[2], ctx->a[2]);
		mb_log_printf("  A3=%08x (%u)\n", ctx->a[3], ctx->a[3]);
		mb_log_printf("  A4=%08x (%u)\n", ctx->a[4], ctx->a[4]);
		mb_log_printf("  A5=%08x (%u)\n", ctx->a[5], ctx->a[5]);
		mb_log_printf("  A6=%08x (%u)\n", ctx->a[6], ctx->a[6]);
	} else {
		mb_log_puts("  no exception context\n");
	}

	mb_board_exit(1);
}
