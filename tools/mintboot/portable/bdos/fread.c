#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fread(uint16_t handle, uint32_t cnt, void *buf)
{
	return mb_fat_fread(handle, cnt, buf);
}
