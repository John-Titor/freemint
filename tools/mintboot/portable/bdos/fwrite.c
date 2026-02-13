#include "mintboot/mb_rom.h"
#include "mintboot/mb_portable.h"

long mb_bdos_fwrite(uint16_t handle, uint32_t cnt, void *buf)
{
	mb_panic("Fwrite(handle=%u, cnt=%u, buf=%08x)", (uint32_t)handle, cnt,
		 (uint32_t)(uintptr_t)buf);
	return -1;
}
