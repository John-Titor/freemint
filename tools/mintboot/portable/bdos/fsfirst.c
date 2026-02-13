#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fsfirst(const char *filespec, uint16_t attr)
{
	return mb_fat_fsfirst(filespec, attr);
}
