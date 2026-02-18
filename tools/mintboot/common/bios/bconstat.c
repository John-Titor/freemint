#include "mintboot/mb_rom.h"
#include "mintboot/mb_xbios_iorec.h"

long mb_bios_bconstat(uint16_t dev)
{
	if (dev == 2)
		return mb_console_iorec_ready() ? -1 : 0;

	mb_panic("Bconstat(dev=%u)", (uint32_t)dev);
}
