#include "mintboot/mb_rom.h"
#include "mintboot/mb_portable.h"

long mb_bios_bconout(uint16_t dev, uint16_t c)
{
	mb_panic("Bconout(dev=%u, c=%u)", (uint32_t)dev, (uint32_t)c);
	return -1;
}
