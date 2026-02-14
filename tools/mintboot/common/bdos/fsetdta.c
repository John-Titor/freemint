#include "mintboot/mb_rom.h"
#include "mintboot/mb_bdos_state.h"

long mb_bdos_fsetdta(void *dta)
{
	mb_bdos_set_dta(dta);
	return 0;
}
