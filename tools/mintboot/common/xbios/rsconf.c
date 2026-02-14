#include "mintboot/mb_rom.h"
#include "mintboot/mb_xbios_iorec.h"

long mb_xbios_rsconf(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs,
		 uint16_t ts, uint16_t sc)
{
	struct mb_ext_iorec *iorec = mb_xbios_iorec_get();

	if (baud != 0xffffu)
		iorec->baudrate = (uint8_t)baud;
	if (flow != 0xffffu)
		iorec->flowctrl = (uint8_t)flow;
	if (uc != 0xffffu)
		iorec->ucr = (uint8_t)uc;
	(void)baud;
	(void)flow;
	(void)uc;
	(void)rs;
	(void)ts;
	(void)sc;
	return 0;
}
