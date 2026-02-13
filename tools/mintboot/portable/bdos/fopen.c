#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fopen(const char *filename, uint16_t mode)
{
	return mb_fat_fopen(filename, mode);
}
