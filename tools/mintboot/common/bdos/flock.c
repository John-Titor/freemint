#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"

long mb_bdos_flock(uint16_t handle, uint16_t mode, int32_t start, int32_t length)
{
	(void)handle;
	(void)mode;
	(void)start;
	(void)length;
	return MB_ERR_INVFN;
}
