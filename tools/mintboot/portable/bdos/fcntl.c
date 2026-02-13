#include "mintboot/mb_rom.h"
#include "mintboot/mb_portable.h"

long mb_bdos_fcntl(uint16_t f, uint32_t arg, uint16_t cmd)
{
	mb_panic("Fcntl(f=%u, arg=%08x, cmd=%u)", (uint32_t)f, arg, (uint32_t)cmd);
	return -1;
}
