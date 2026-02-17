#include "mintboot/mb_aes_vdi.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_debug.h"

struct aespb
{
	uint16_t *contrl;
	uint16_t *global;
	uint16_t *intin;
	uint16_t *intout;
	uint32_t *addrin;
	uint32_t *addrout;
};

static long
aes_dispatch(struct aespb *pb)
{
	switch (pb->contrl[0]) {
	case 0x0a:					// appl_init
		// only handle the auto-folder case, i.e. do nothing
		return -1;
	default:
		return MB_ERR_INVFN;
	}
}

static long
vdi_dispatch(void *args)
{
	(void)args;
	return MB_ERR_INVFN;
}

long mb_aes_vdi_dispatch(uint16_t function, void *args, uint32_t *retaddr)
{
	long ret;

	mb_debug_aes_vdi_enter(function, args, retaddr);

	switch (function) {
	case 0x00:
		mb_bdos_pterm0();
	case 0x73:
		ret = vdi_dispatch(args);
		break;
	case 0xc8:
		ret = aes_dispatch(args);
		break;
	default:
		ret = MB_ERR_INVFN;
		break;
	}

	mb_debug_aes_vdi_exit(function, args, retaddr, ret);
	return ret;
}
