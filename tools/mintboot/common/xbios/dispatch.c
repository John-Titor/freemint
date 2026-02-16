#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_debug.h"

long mb_xbios_dispatch(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	long ret;

	mb_debug_xbios_enter(fnum, args, retaddr);

	switch (fnum) {
	case 0x00:
		ret = mb_xbios_initmous(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
		break;
	case 0x02:
		ret = mb_xbios_physbase();
		break;
	case 0x03:
		ret = mb_xbios_logbase();
		break;
	case 0x04:
		ret = mb_xbios_getrez();
		break;
	case 0x0e:
		ret = mb_xbios_iorec(mb_arg16(args, 0));
		break;
	case 0x0f:
		ret = mb_xbios_rsconf(mb_arg16(args, 0), mb_arg16(args, 1), mb_arg16(args, 2),
				      mb_arg16(args, 3), mb_arg16(args, 4), mb_arg16(args, 5));
		break;
	case 0x10:
		ret = mb_xbios_keytbl(mb_arg32w(args, 0), mb_arg32w(args, 2), mb_arg32w(args, 4));
		break;
	case 0x15:
		ret = mb_xbios_cursconf(mb_arg16(args, 0), mb_arg16(args, 1));
		break;
	case 0x16:
		ret = mb_xbios_settime(mb_arg32w(args, 0));
		break;
	case 0x17:
		ret = mb_xbios_gettime();
		break;
	case 0x18:
		ret = mb_xbios_bioskeys();
		break;
	case 0x1d:
		ret = mb_xbios_offgibit(mb_arg16(args, 0));
		break;
	case 0x1e:
		ret = mb_xbios_ongibit(mb_arg16(args, 0));
		break;
	case 0x20:
		ret = mb_xbios_dosound(mb_arg32w(args, 0));
		break;
	case 0x22:
		ret = mb_xbios_kbdvbase();
		break;
	case 0x23:
		ret = mb_xbios_kbrate(mb_arg16(args, 0), mb_arg16(args, 1));
		break;
	case 0x25:
		ret = mb_xbios_vsync();
		break;
	case 0x26:
		ret = mb_xbios_supexec(mb_arg32w(args, 0));
		break;
	case 0x2c:
		ret = mb_xbios_bconmap(mb_arg16(args, 0));
		break;
	case 0x05:
		ret = mb_xbios_vsetscreen(mb_arg32w(args, 0), mb_arg32w(args, 2),
					  mb_arg16(args, 4), mb_arg16(args, 5));
		break;
	default:
		mb_log_printf("xbios: unhandled 0x%04x\n", (uint32_t)fnum);
		ret = MB_ERR_INVFN;
		break;
	}

	mb_debug_xbios_exit(fnum, args, retaddr, ret);
	return ret;
}
