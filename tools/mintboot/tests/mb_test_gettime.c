#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mb_tests_internal.h"

void mb_tests_gettime(void)
{
	uint32_t dt = (uint32_t)Gettime();
	uint16_t date = (uint16_t)(dt >> 16);
	uint16_t time = (uint16_t)dt;
	uint16_t year = (uint16_t)(1980u + ((date >> 9) & 0x7fu));
	uint16_t month = (uint16_t)((date >> 5) & 0x0fu);
	uint16_t day = (uint16_t)(date & 0x1fu);
	uint16_t hour = (uint16_t)((time >> 11) & 0x1fu);
	uint16_t minute = (uint16_t)((time >> 5) & 0x3fu);
	uint16_t second = (uint16_t)((time & 0x1fu) * 2u);
	uint32_t packed_min;
	uint32_t packed_got;

	if (month < 1 || month > 12)
		mb_panic("Gettime: month=%u", (uint32_t)month);
	if (day < 1 || day > 31)
		mb_panic("Gettime: day=%u", (uint32_t)day);
	if (hour > 23)
		mb_panic("Gettime: hour=%u", (uint32_t)hour);
	if (minute > 59)
		mb_panic("Gettime: minute=%u", (uint32_t)minute);
	if (second > 59)
		mb_panic("Gettime: second=%u", (uint32_t)second);

	packed_min = (uint32_t)(((2026u - 1980u) << 9) | (1u << 5) | 1u);
	packed_got = (uint32_t)(((year - 1980u) << 9) | (month << 5) | day);
	if (packed_got < packed_min)
		mb_panic("Gettime: date=%u/%u/%u below 2026-01-01",
			 (uint32_t)year, (uint32_t)month, (uint32_t)day);
}
