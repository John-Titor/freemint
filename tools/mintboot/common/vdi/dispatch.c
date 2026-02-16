#include "mintboot/mb_vdi.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_debug.h"

long mb_vdi_dispatch(uint16_t function, uint16_t *args, uint32_t *retaddr)
{
	long ret;

	mb_check_vector20("vdi");
	mb_debug_vdi_enter(function, args, retaddr);
	(void)args;

	switch (function) {
	case 0:
		ret = mb_bdos_pterm0();
		break;
	default:
		ret = MB_ERR_INVFN;
		break;
	}

	mb_debug_vdi_exit(function, args, retaddr, ret);
	return ret;
}
