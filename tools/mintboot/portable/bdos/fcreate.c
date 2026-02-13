#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fcreate(const char *fn, uint16_t mode)
{
	return mb_fat_fcreate(fn, mode);
}
