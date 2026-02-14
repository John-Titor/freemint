#include "mintboot/mb_vdi.h"
#include "mintboot/mb_rom.h"

long mb_vdi_dispatch(uint16_t function, uint16_t *args)
{
	(void)args;

	switch (function) {
	case 0:
		return mb_bdos_pterm0();
	default:
		return MB_ERR_INVFN;
	}
}
