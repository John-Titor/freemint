#include "mintboot/mb_board.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_kernel.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_cpu.h"

#include <stdarg.h>

static void mb_log_putc(int ch)
{
	if (ch == '\n')
		mb_log_putc('\r');

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

	mb_log_puts("\nmintboot panic: ");
	va_start(ap, fmt);
	mb_log_vprintf(fmt, ap);
	va_end(ap);
	mb_log_puts("\n");

	mb_board_exit(1);
}
