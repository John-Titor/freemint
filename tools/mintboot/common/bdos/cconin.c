#include "mintboot/mb_rom.h"

long mb_bdos_cconin(void)
{
	return mb_bios_bconin(2);
}
