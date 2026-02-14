#include "mintboot/mb_rom.h"

long mb_bdos_cconin(void)
{
	return mb_rom_dispatch.bconin(2);
}
