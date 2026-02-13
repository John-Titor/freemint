#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fdatime(uint32_t timeptr, uint16_t handle, uint16_t rwflag)
{
	return mb_fat_fdatime(timeptr, handle, rwflag);
}
