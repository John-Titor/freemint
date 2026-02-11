#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include <string.h>

extern struct mb_boot_info mb_virt_boot;

static long mb_virt_rom_bconout(uint16_t dev, uint16_t c)
{
	(void)dev;
	mb_board_console_putc((int)(c & 0xff));
	return 0;
}

static long mb_virt_rom_bconstat(uint16_t dev)
{
	(void)dev;
	return mb_board_console_poll() ? 1 : 0;
}

static long mb_virt_rom_bconin(uint16_t dev)
{
	(void)dev;
	return mb_board_console_getc();
}

static long mb_virt_rom_rwabs(uint16_t rwflag, uint32_t buf, uint16_t count,
			      uint16_t recno, uint16_t dev)
{
	uint32_t offset;
	uint32_t size;
	uint32_t end;
	uint8_t *ramdisk;
	uint8_t *data;

	ramdisk = (uint8_t *)mb_virt_boot.ramdisk_base;
	if (!ramdisk)
		return -1;

	if (dev != 2)
		return -1;

	offset = (uint32_t)recno * 512u;
	size = mb_virt_boot.ramdisk_size;
	end = offset + ((uint32_t)count * 512u);
	if (end > size)
		return -1;

	data = (uint8_t *)(uintptr_t)buf;
	if ((rwflag & 1u) == 0)
		memcpy(data, ramdisk + offset, end - offset);
	else
		memcpy(ramdisk + offset, data, end - offset);

	return 0;
}

const struct mb_rom_dispatch mb_rom_dispatch = {
	.fsetdta = mb_rom_fsetdta,
	.fgetdta = mb_rom_fgetdta,
	.dfree = mb_rom_dfree,
	.dcreate = mb_rom_dcreate,
	.ddelete = mb_rom_ddelete,
	.fcreate = mb_rom_fcreate,
	.fopen = mb_rom_fopen,
	.fclose = mb_rom_fclose,
	.fread = mb_rom_fread,
	.fwrite = mb_rom_fwrite,
	.fdelete = mb_rom_fdelete,
	.fseek = mb_rom_fseek,
	.fattrib = mb_rom_fattrib,
	.fsfirst = mb_rom_fsfirst,
	.fsnext = mb_rom_fsnext,
	.frename = mb_rom_frename,
	.fdatime = mb_rom_fdatime,
	.flock = mb_rom_flock,
	.fcntl = mb_rom_fcntl,
	.bconstat = mb_virt_rom_bconstat,
	.bconin = mb_virt_rom_bconin,
	.bconout = mb_virt_rom_bconout,
	.rwabs = mb_virt_rom_rwabs,
	.setexc = mb_rom_setexc,
	.getbpb = mb_rom_getbpb,
	.bcostat = mb_rom_bcostat,
	.drvmap = mb_rom_drvmap,
	.kbshift = mb_rom_kbshift,
	.initmous = mb_rom_initmous,
	.getrez = mb_rom_getrez,
	.iorec = mb_rom_iorec,
	.rsconf = mb_rom_rsconf,
	.keytbl = mb_rom_keytbl,
	.cursconf = mb_rom_cursconf,
	.settime = mb_rom_settime,
	.gettime = mb_rom_gettime,
	.bioskeys = mb_rom_bioskeys,
	.offgibit = mb_rom_offgibit,
	.ongibit = mb_rom_ongibit,
	.dosound = mb_rom_dosound,
	.kbdvbase = mb_rom_kbdvbase,
	.vsync = mb_rom_vsync,
	.bconmap = mb_rom_bconmap,
	.vsetscreen = mb_rom_vsetscreen,
	.kbrate = mb_rom_kbrate,
};
