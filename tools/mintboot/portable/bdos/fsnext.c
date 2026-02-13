#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fsnext(void)
{
	return mb_fat_fsnext();
}
