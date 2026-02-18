#include "mintboot/mb_rom.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_xbios_iorec.h"

long mb_bios_bconin(uint16_t dev)
{
	if (dev == 2)
		return (long)(uint8_t)mb_console_iorec_getc();

	mb_panic("Bconin(dev=%u)", (uint32_t)dev);
}
