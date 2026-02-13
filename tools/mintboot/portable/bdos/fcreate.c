#include "mintboot/mb_rom.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_portable.h"

long mb_bdos_fcreate(const char *fn, uint16_t mode)
{
	mb_panic("Fcreate(fn=%08x, mode=%u, \"%s\")", (uint32_t)(uintptr_t)fn,
		 (uint32_t)mode, mb_guarded_str(fn));
	return -1;
}
