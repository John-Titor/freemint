#include "mintboot/mb_rom.h"
#include "mintboot/mb_bdos_state.h"

void *mb_bdos_fgetdta(void)
{
	return mb_bdos_get_dta();
}
