#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_debug.h"

long mb_bios_dispatch(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	long ret;

	mb_debug_bios_enter(fnum, args, retaddr);

	switch (fnum) {
	case 0x01:
		ret = mb_bios_bconstat(mb_arg16(args, 0));
		break;
	case 0x02:
		ret = mb_bios_bconin(mb_arg16(args, 0));
		break;
	case 0x03:
		ret = mb_rom_dispatch.bconout(mb_arg16(args, 0), mb_arg16(args, 1));
		break;
	case 0x04:
		ret = mb_rom_dispatch.rwabs(mb_arg16(args, 0),
					    (void *)(uintptr_t)mb_arg32w(args, 1),
					    mb_arg16(args, 3), mb_arg16(args, 4),
					    mb_arg16(args, 5));
		break;
	case 0x05:
		ret = mb_bios_setexc(mb_arg16(args, 0), mb_arg32w(args, 1));
		break;
	case 0x07:
		ret = mb_bios_getbpb(mb_arg16(args, 0));
		break;
	case 0x08:
		ret = mb_rom_dispatch.bcostat(mb_arg16(args, 0));
		break;
	case 0x0a:
		ret = mb_rom_dispatch.drvmap();
		break;
	case 0x0b:
		ret = mb_bios_kbshift(mb_arg16(args, 0));
		break;
	default:
		if (fnum != 0xffffu)
			mb_log_printf("bios: unhandled 0x%04x\n", (uint32_t)fnum);
		ret = MB_ERR_INVFN;
		break;
	}

	mb_debug_bios_exit(fnum, args, retaddr, ret);
	return ret;
}
