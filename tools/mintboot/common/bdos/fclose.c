#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fclose(uint16_t handle)
{
	return mb_fat_fclose(handle);
}
