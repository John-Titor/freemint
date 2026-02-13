#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"
#include "mintboot/mb_errors.h"

long mb_bdos_dfree(uint32_t buf, uint16_t d)
{
	if (d > 25)
		return MB_ERR_DRIVE;
	return mb_fat_dfree(buf, d);
}
