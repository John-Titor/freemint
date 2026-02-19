#include "mintboot/mb_board.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mb_virt_mmio.h"
#include "mb_virt_blk.h"

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

static long mb_virt_rom_rwabs(uint16_t rwflag, void *buf, uint16_t count,
			      uint16_t recno, uint16_t dev)
{
	return mb_virt_blk_rwabs(rwflag, buf, count, recno, dev);
}

static long mb_virt_rom_drvmap(void)
{
	return 1u << 2;
}

const struct mb_rom_dispatch mb_rom_dispatch = {
	.bconout = mb_virt_rom_bconout,
	.rwabs = mb_virt_rom_rwabs,
	.bcostat = mb_bios_bcostat,
	.drvmap = mb_virt_rom_drvmap,
	.settime = mb_xbios_settime,
	.gettime = mb_virt_rom_gettime,
};
