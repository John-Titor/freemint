#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_debug.h"

long mb_bdos_dispatch(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	long ret;

	mb_debug_bdos_enter(fnum, args, retaddr);

	switch (fnum) {
	case 0x000:
		ret = mb_bdos_pterm0();
		break;
	case 0x001:
		ret = mb_bdos_cconin();
		break;
	case 0x009:
		ret = mb_bdos_cconws((const char *)(uintptr_t)mb_arg32w(args, 0));
		break;
	case 0x019:
		ret = mb_bdos_dgetdrv();
		break;
	case 0x01a:
		ret = mb_bdos_fsetdta((void *)(uintptr_t)mb_arg32w(args, 0));
		break;
	case 0x00e:
		ret = mb_bdos_dsetdrv(mb_arg16(args, 0));
		break;
	case 0x047:
		ret = mb_bdos_dgetpath((char *)(uintptr_t)mb_arg32w(args, 0),
				       mb_arg16(args, 2));
		break;
	case 0x036:
		ret = mb_bdos_dfree(mb_arg32w(args, 0), mb_arg16(args, 2));
		break;
	case 0x03b:
		ret = mb_bdos_dsetpath((const char *)(uintptr_t)mb_arg32w(args, 0));
		break;
	case 0x03c:
		ret = mb_bdos_fcreate((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
		break;
	case 0x03d:
		ret = mb_bdos_fopen((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
		break;
	case 0x03e:
		ret = mb_bdos_fclose(mb_arg16(args, 0));
		break;
	case 0x03f:
		ret = mb_bdos_fread(mb_arg16(args, 0), mb_arg32w(args, 1),
				    (void *)(uintptr_t)mb_arg32w(args, 3));
		break;
	case 0x040:
		ret = mb_bdos_fwrite(mb_arg16(args, 0), mb_arg32w(args, 1),
				     (void *)(uintptr_t)mb_arg32w(args, 3));
		break;
	case 0x041:
		ret = mb_bdos_fdelete((const char *)(uintptr_t)mb_arg32w(args, 0));
		break;
	case 0x042:
		ret = mb_bdos_fseek((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2), mb_arg16(args, 3));
		break;
	case 0x044:
		ret = mb_bdos_mxalloc((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2));
		break;
	case 0x04a:
		ret = mb_bdos_mshrink(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
		break;
	case 0x05c:
		ret = mb_bdos_flock(mb_arg16(args, 0), mb_arg16(args, 1),
				    (int32_t)mb_arg32w(args, 2), (int32_t)mb_arg32w(args, 4));
		break;
	case 0x104:
		ret = mb_bdos_fcntl(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg16(args, 3));
		break;
	default:
		if (fnum != 0xffffu)
			mb_log_printf("bdos: unhandled 0x%04x\n", (uint32_t)fnum);
		ret = MB_ERR_INVFN;
		break;
	}

	mb_debug_bdos_exit(fnum, args, retaddr, ret);
	return ret;
}
