#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include <string.h>

extern struct mb_boot_info mb_virt_boot;

#define VIRT_GF_RTC_MMIO_BASE  0xff006000u
#define GOLDFISH_RTC_TIME_LOW  0x00
#define GOLDFISH_RTC_TIME_HIGH 0x04

static inline uint32_t mb_mmio_read32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static int mb_is_leap(int year)
{
	if ((year % 4) != 0)
		return 0;
	if ((year % 100) != 0)
		return 1;
	return (year % 400) == 0;
}

static uint32_t mb_dos_datetime_from_epoch(uint64_t secs)
{
	static const uint8_t days_in_month[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	uint64_t days = secs / 86400u;
	uint32_t rem = (uint32_t)(secs % 86400u);
	int year = 1970;
	int month = 1;
	int day;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
	uint16_t date;
	uint16_t time;

	hour = rem / 3600u;
	rem %= 3600u;
	minute = rem / 60u;
	second = rem % 60u;

	for (;;) {
		uint32_t year_days = mb_is_leap(year) ? 366u : 365u;
		if (days < year_days)
			break;
		days -= year_days;
		year++;
	}

	for (month = 1; month <= 12; month++) {
		uint32_t mdays = days_in_month[month - 1];
		if (month == 2 && mb_is_leap(year))
			mdays = 29;
		if (days < mdays)
			break;
		days -= mdays;
	}

	day = (int)days + 1;
	if (year < 1980) {
		year = 1980;
		month = 1;
		day = 1;
		hour = 0;
		minute = 0;
		second = 0;
	}

	date = (uint16_t)(((year - 1980) << 9) | (month << 5) | day);
	time = (uint16_t)((hour << 11) | (minute << 5) | (second / 2u));
	return ((uint32_t)date << 16) | time;
}

static long mb_virt_rom_gettime(void)
{
	uint32_t low = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_LOW);
	uint32_t high = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_HIGH);
	uint64_t ns = ((uint64_t)high << 32) | low;
	uint64_t secs = ns / 1000000000ull;

	return (long)mb_dos_datetime_from_epoch(secs);
}

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

static long mb_virt_rom_rwabs(uint16_t rwflag, void *buf, uint16_t count,
			      uint16_t recno, uint16_t dev)
{
	uint32_t offset;
	uint32_t size;
	uint32_t end;
	uint8_t *ramdisk;

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

	if ((rwflag & 1u) == 0)
		memcpy(buf, ramdisk + offset, end - offset);
	else
		memcpy(ramdisk + offset, buf, end - offset);

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
	.gettime = mb_virt_rom_gettime,
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
