#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_frename(uint16_t zero, const char *oldname, const char *newname)
{
	return mb_fat_frename(zero, oldname, newname);
}
