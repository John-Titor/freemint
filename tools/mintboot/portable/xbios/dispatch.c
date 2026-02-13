#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_portable.h"

long mb_xbios_dispatch(uint16_t fnum, uint16_t *args)
{
	switch (fnum) {
	case 0x00:
		return mb_xbios_initmous(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
	case 0x04:
		return mb_xbios_getrez();
	case 0x0e:
		return mb_xbios_iorec(mb_arg16(args, 0));
	case 0x0f:
		return mb_xbios_rsconf(mb_arg16(args, 0), mb_arg16(args, 1), mb_arg16(args, 2),
				     mb_arg16(args, 3), mb_arg16(args, 4), mb_arg16(args, 5));
	case 0x10:
		return mb_xbios_keytbl(mb_arg32w(args, 0), mb_arg32w(args, 2), mb_arg32w(args, 4));
	case 0x15:
		return mb_xbios_cursconf(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x16:
		return mb_xbios_settime(mb_arg32w(args, 0));
	case 0x17:
		return mb_xbios_gettime();
	case 0x18:
		return mb_xbios_bioskeys();
	case 0x1d:
		return mb_xbios_offgibit(mb_arg16(args, 0));
	case 0x1e:
		return mb_xbios_ongibit(mb_arg16(args, 0));
	case 0x20:
		return mb_xbios_dosound(mb_arg32w(args, 0));
	case 0x22:
		return mb_xbios_kbdvbase();
	case 0x23:
		return mb_xbios_kbrate(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x25:
		return mb_xbios_vsync();
	case 0x26:
		return mb_xbios_supexec(mb_arg32w(args, 0));
	case 0x2c:
		return mb_xbios_bconmap(mb_arg16(args, 0));
	case 0x05:
		return mb_xbios_vsetscreen(mb_arg32w(args, 0), mb_arg32w(args, 2),
				 mb_arg16(args, 4), mb_arg16(args, 5));
	default:
		mb_log_printf("xbios: unhandled 0x%04x", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}
