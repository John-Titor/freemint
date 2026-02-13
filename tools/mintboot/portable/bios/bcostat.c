#include "mintboot/mb_rom.h"

long mb_bios_bcostat(uint16_t dev)
{
	if (dev == 2)
		return -1;
	return 0;
}
