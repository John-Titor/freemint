#include "mintboot/mb_rom.h"

long mb_xbios_cursconf(uint16_t rate, uint16_t attr)
{
	(void)attr;
	if (rate == 5 || rate == 7)
		return 20;
	return 0;
}
