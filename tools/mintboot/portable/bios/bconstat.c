#include "mintboot/mb_rom.h"
#include "mintboot/mb_portable.h"

long mb_bios_bconstat(uint16_t dev)
{
	mb_panic("Bconstat(dev=%u)", (uint32_t)dev);
	return -1;
}
