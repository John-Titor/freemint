#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_fseek(int32_t where, uint16_t handle, uint16_t how)
{
	return mb_fat_fseek(handle, where, how);
}
