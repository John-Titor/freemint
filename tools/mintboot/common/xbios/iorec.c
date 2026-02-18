#include "mintboot/mb_rom.h"
#include "mintboot/mb_xbios_iorec.h"

enum { mb_rs232_bufsize = 256 };
enum { mb_console_bufsize = 256 };

static uint8_t mb_iorec_ibuf[mb_rs232_bufsize];
static uint8_t mb_iorec_obuf[mb_rs232_bufsize];
static struct mb_ext_iorec mb_iorec;

static uint8_t mb_console_buf[mb_console_bufsize];
static struct mb_iorec mb_console_iorec;
static int inited;

struct mb_ext_iorec *mb_xbios_iorec_get(void)
{
	if (!inited) {
		mb_iorec.in.buf = mb_iorec_ibuf;
		mb_iorec.in.size = mb_rs232_bufsize;
		mb_iorec.in.head = 0;
		mb_iorec.in.tail = 0;
		mb_iorec.in.low = mb_rs232_bufsize / 4;
		mb_iorec.in.high = (mb_rs232_bufsize * 3) / 4;
		mb_iorec.out.buf = mb_iorec_obuf;
		mb_iorec.out.size = mb_rs232_bufsize;
		mb_iorec.out.head = 0;
		mb_iorec.out.tail = 0;
		mb_iorec.out.low = mb_rs232_bufsize / 4;
		mb_iorec.out.high = (mb_rs232_bufsize * 3) / 4;
		mb_iorec.baudrate = 0;
		mb_iorec.flowctrl = 0;
		mb_iorec.ucr = 0;
		mb_iorec.datamask = 0;
		mb_iorec.wr5 = 0;

		mb_console_iorec.buf = mb_console_buf;
		mb_console_iorec.size = mb_console_bufsize;
		mb_console_iorec.head = 0;
		mb_console_iorec.tail = 0;
		mb_console_iorec.low = mb_console_bufsize / 4;
		mb_console_iorec.high = (mb_console_bufsize * 3) / 4;
		inited = 1;
	}

	return &mb_iorec;
}

struct mb_iorec *mb_console_iorec_get(void)
{
	(void)mb_xbios_iorec_get();
	return &mb_console_iorec;
}

int mb_console_iorec_putc(uint8_t ch)
{
	struct mb_iorec *iorec = mb_console_iorec_get();
	uint16_t head = iorec->head;
	uint16_t next = (uint16_t)(head + 1u);

	if (next >= iorec->size)
		next = 0;
	if (next == iorec->tail)
		return 0;

	iorec->buf[head] = ch;
	iorec->head = next;
	return 1;
}

int mb_console_iorec_getc(void)
{
	struct mb_iorec *iorec = mb_console_iorec_get();
	uint16_t tail;
	uint8_t ch;

	while (iorec->tail == iorec->head) {
	}

	tail = iorec->tail;
	ch = iorec->buf[tail];
	tail = (uint16_t)(tail + 1u);
	if (tail >= iorec->size)
		tail = 0;
	iorec->tail = tail;
	return (int)ch;
}

int mb_console_iorec_ready(void)
{
	struct mb_iorec *iorec = mb_console_iorec_get();
	return iorec->head != iorec->tail;
}

long mb_xbios_iorec(uint16_t io_dev)
{
	if (io_dev == 0)
		return (long)(uintptr_t)mb_xbios_iorec_get();
	if (io_dev == 1)
		return (long)(uintptr_t)mb_console_iorec_get();
	return -1;
}
