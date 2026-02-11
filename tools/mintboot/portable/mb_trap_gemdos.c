#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_lowmem.h"

#include <stdint.h>

/* GEMDOS (trap #1) stubs */
#define MB_EMUTOS_DTA_SIZE 44

static uint8_t mb_default_dta[MB_EMUTOS_DTA_SIZE];
static void *mb_gemdos_dta = mb_default_dta;

long mb_rom_fsetdta(void *dta)
{
	mb_gemdos_dta = dta;
	return 0;
}

void *mb_rom_fgetdta(void)
{
	return mb_gemdos_dta;
}
long mb_rom_dfree(uint32_t buf, uint16_t d)
{
	if (d > 25)
		return MB_ERR_DRIVE;
	return mb_fat_dfree(buf, d);
}

long mb_rom_dcreate(const char *path)
{
	return mb_fat_dcreate(path);
}

long mb_rom_ddelete(const char *path)
{
	return mb_fat_ddelete(path);
}

long mb_rom_pterm0(void)
{
	mb_log_puts("Pterm0\r\n");
	mb_board_exit(0);
	return 0;
}

long mb_rom_cconin(void)
{
	return mb_rom_dispatch.bconin(2);
}

long mb_rom_cconws(const char *buf)
{
	const char *s = mb_guarded_str(buf);

	mb_log_puts(s);
	return 0;
}

long mb_rom_mxalloc(int32_t amount, uint16_t mode)
{
	uint32_t membot = *mb_lm_membot();
	uint32_t memtop = *mb_lm_memtop();
	uint32_t avail = 0;
	uint32_t alloc;

	if (memtop > membot)
		avail = memtop - membot;

	if (amount < 0) {
		if (mode == 1 || mode == 3)
			return 0;
		return (long)avail;
	}

	if (amount == 0)
		return 0;
	alloc = (uint32_t)amount;
	alloc = (alloc + 3u) & ~3u;
	if (alloc > avail)
		return MB_ERR_NHNDL;
	*mb_lm_membot() = membot + alloc;
	return 0;
}

long mb_rom_fcreate(const char *fn, uint16_t mode)
{
	mb_panic("Fcreate(fn=%08x, mode=%u, \"%s\")", (uint32_t)(uintptr_t)fn,
		 (uint32_t)mode, mb_guarded_str(fn));
	return -1;
}

long mb_rom_fopen(const char *filename, uint16_t mode)
{
	return mb_fat_fopen(filename, mode);
}

long mb_rom_fclose(uint16_t handle)
{
	return mb_fat_fclose(handle);
}

long mb_rom_fread(uint16_t handle, uint32_t cnt, void *buf)
{
	return mb_fat_fread(handle, cnt, buf);
}

long mb_rom_fwrite(uint16_t handle, uint32_t cnt, void *buf)
{
	mb_panic("Fwrite(handle=%u, cnt=%u, buf=%08x)", (uint32_t)handle, cnt,
		 (uint32_t)(uintptr_t)buf);
	return -1;
}

long mb_rom_fdelete(const char *fn)
{
	mb_panic("Fdelete(fn=%08x, \"%s\")", (uint32_t)(uintptr_t)fn,
		 mb_guarded_str(fn));
	return -1;
}

long mb_rom_fseek(int32_t where, uint16_t handle, uint16_t how)
{
	return mb_fat_fseek(handle, where, how);
}

long mb_rom_fattrib(const char *fn, uint16_t rwflag, uint16_t attr)
{
	return mb_fat_fattrib(fn, rwflag, attr);
}

long mb_rom_fsfirst(const char *filespec, uint16_t attr)
{
	return mb_fat_fsfirst(filespec, attr);
}

long mb_rom_fsnext(void)
{
	return mb_fat_fsnext();
}

long mb_rom_frename(uint16_t zero, const char *oldname, const char *newname)
{
	return mb_fat_frename(zero, oldname, newname);
}

long mb_rom_fdatime(uint32_t timeptr, uint16_t handle, uint16_t rwflag)
{
	return mb_fat_fdatime(timeptr, handle, rwflag);
}

long mb_rom_flock(uint16_t handle, uint16_t mode, int32_t start, int32_t length)
{
	(void)handle;
	(void)mode;
	(void)start;
	(void)length;
	return MB_ERR_INVFN;
}

long mb_rom_fcntl(uint16_t f, uint32_t arg, uint16_t cmd)
{
	mb_panic("Fcntl(f=%u, arg=%08x, cmd=%u)", (uint32_t)f, arg, (uint32_t)cmd);
	return -1;
}

long mb_rom_mshrink(uint16_t zero, uint32_t base, uint32_t len)
{
	mb_log_printf("Mshrink(zero=%u, base=%08x, len=%u)\r\n",
		      (uint32_t)zero, base, len);
	return MB_ERR_INVFN;
}

long mb_rom_gemdos_dispatch(uint16_t fnum, uint16_t *args)
{
	switch (fnum) {
	case 0x000:
		return mb_rom_pterm0();
	case 0x001:
		return mb_rom_cconin();
	case 0x009:
		return mb_rom_cconws((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x01a:
		return mb_rom_fsetdta((void *)(uintptr_t)mb_arg32w(args, 0));
	case 0x036:
		return mb_rom_dfree(mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x039:
		return mb_rom_dcreate((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x03a:
		return mb_rom_ddelete((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x03c:
		return mb_rom_fcreate((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x03d:
		return mb_rom_fopen((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x03e:
		return mb_rom_fclose(mb_arg16(args, 0));
	case 0x03f:
		return mb_rom_fread(mb_arg16(args, 0), mb_arg32w(args, 1),
					     (void *)(uintptr_t)mb_arg32w(args, 3));
	case 0x040:
		return mb_rom_fwrite(mb_arg16(args, 0), mb_arg32w(args, 1),
					      (void *)(uintptr_t)mb_arg32w(args, 3));
	case 0x041:
		return mb_rom_fdelete((const char *)(uintptr_t)mb_arg32w(args, 0));
	case 0x042:
		return mb_rom_fseek((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2), mb_arg16(args, 3));
	case 0x043:
		return mb_rom_fattrib((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2), mb_arg16(args, 3));
	case 0x03b:
		return mb_rom_mxalloc((int32_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x04a:
		return mb_rom_mshrink(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
	case 0x04e:
		return mb_rom_fsfirst((const char *)(uintptr_t)mb_arg32w(args, 0), mb_arg16(args, 2));
	case 0x04f:
		return mb_rom_fsnext();
	case 0x056:
		return mb_rom_frename(mb_arg16(args, 0), (const char *)(uintptr_t)mb_arg32w(args, 1), (const char *)(uintptr_t)mb_arg32w(args, 3));
	case 0x057:
		return mb_rom_fdatime(mb_arg32w(args, 0), mb_arg16(args, 2), mb_arg16(args, 3));
	case 0x05c:
		return mb_rom_flock(mb_arg16(args, 0), mb_arg16(args, 1), (int32_t)mb_arg32w(args, 2), (int32_t)mb_arg32w(args, 4));
	case 0x104:
		return mb_rom_fcntl(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg16(args, 3));
	default:
		mb_log_printf("gemdos: unhandled 0x%04x", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}
