#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fattrib(const char *fn, uint16_t rwflag, uint16_t attr)
{
	return mb_fat_fattrib(fn, rwflag, attr);
}
