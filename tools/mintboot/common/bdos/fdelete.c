#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fdelete(const char *fn)
{
	return mb_fat_fdelete(fn);
}
