#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_portable.h"

long mb_bios_dispatch(uint16_t fnum, uint16_t *args)
{
	switch (fnum) {
	case 0x01:
		return mb_rom_dispatch.bconstat(mb_arg16(args, 0));
	case 0x02:
		return mb_rom_dispatch.bconin(mb_arg16(args, 0));
	case 0x03:
		return mb_rom_dispatch.bconout(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x04:
		return mb_rom_dispatch.rwabs(mb_arg16(args, 0),
				     (void *)(uintptr_t)mb_arg32w(args, 1),
				     mb_arg16(args, 3), mb_arg16(args, 4),
				     mb_arg16(args, 5));
	case 0x05:
		return mb_bios_setexc(mb_arg16(args, 0), mb_arg32w(args, 1));
	case 0x07:
		return mb_bios_getbpb(mb_arg16(args, 0));
	case 0x08:
		return mb_rom_dispatch.bcostat(mb_arg16(args, 0));
	case 0x0a:
		return mb_rom_dispatch.drvmap();
	case 0x0b:
		return mb_bios_kbshift(mb_arg16(args, 0));
	default:
		if (fnum != 0xffffu)
			mb_log_printf("bios: unhandled 0x%04x\r\n", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}
