#include "mintboot/mb_rom.h"

long mb_xbios_gettime(void)
{
	const uint16_t year = 2026;
	const uint16_t month = 1;
	const uint16_t day = 1;
	uint16_t date;

	date = (uint16_t)(((year - 1980u) << 9) | (month << 5) | day);
	return (long)(((uint32_t)date << 16) | 0u);
}
