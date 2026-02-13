#include <stdint.h>

#include "mintboot/mb_rom.h"

long mb_xbios_supexec(uint32_t func)
{
	long (*entry)(void) = (long (*)(void))(uintptr_t)func;

	if (entry == 0)
		return 0;

	return entry();
}
