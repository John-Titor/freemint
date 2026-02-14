#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_common.h"

long mb_bdos_dispatch(uint16_t fnum, uint16_t *args)
{
	switch (fnum) {
	case 0x000:
		return mb_bdos_pterm0();
	case 0x001:
		return mb_bdos_cconin();
	case 0x009:
		return mb_bdos_cconws((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x019:
		return mb_bdos_dgetdrv();
	case 0x01a:
		return mb_bdos_fsetdta((void *)(uintptr_t)mb_arg32w(args, 0));
	case 0x00e:
		return mb_bdos_dsetdrv(mb_arg16(args, 0));
	case 0x047:
		return mb_bdos_dgetpath((char *)(uintptr_t)mb_arg32w(args, 0),
				       mb_arg16(args, 2));
	case 0x036:
		return mb_bdos_dfree(mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x03b:
		return mb_bdos_dsetpath((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x03c:
		return mb_bdos_fcreate((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x03d:
		return mb_bdos_fopen((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x03e:
		return mb_bdos_fclose(mb_arg16(args, 0));
	case 0x03f:
		return mb_bdos_fread(mb_arg16(args, 0), mb_arg32w(args, 1),
				     (void *)(uintptr_t)mb_arg32w(args, 3));
	case 0x040:
		return mb_bdos_fwrite(mb_arg16(args, 0), mb_arg32w(args, 1),
				      (void *)(uintptr_t)mb_arg32w(args, 3));
	case 0x041:
		return mb_bdos_fdelete((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x042:
		return mb_bdos_fseek((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2), mb_arg16(args, 3));
	case 0x044:
		return mb_bdos_mxalloc((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x04a:
		return mb_bdos_mshrink(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
	case 0x05c:
		return mb_bdos_flock(mb_arg16(args, 0), mb_arg16(args, 1),
				    (int32_t)mb_arg32w(args, 2), (int32_t)mb_arg32w(args, 4));
	case 0x104:
		return mb_bdos_fcntl(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg16(args, 3));
	default:
		if (fnum != 0xffffu)
			mb_log_printf("bdos: unhandled 0x%04x\n", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}
