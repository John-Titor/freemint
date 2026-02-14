#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"

long mb_xbios_physbase(void)
{
	return (long)*mb_lm_v_bas_ad();
}
