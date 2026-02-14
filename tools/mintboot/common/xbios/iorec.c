#include "mintboot/mb_rom.h"
#include "mintboot/mb_xbios_iorec.h"

struct mb_ext_iorec *mb_xbios_iorec_get(void)
{
	enum { mb_rs232_bufsize = 256 };
	static uint8_t mb_iorec_ibuf[mb_rs232_bufsize];
	static uint8_t mb_iorec_obuf[mb_rs232_bufsize];
	static struct mb_ext_iorec mb_iorec;
	static int inited;

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
		inited = 1;
	}

	return &mb_iorec;
}

long mb_xbios_iorec(uint16_t io_dev)
{
	if (io_dev == 0)
		return (long)(uintptr_t)mb_xbios_iorec_get();
	return -1;
}
