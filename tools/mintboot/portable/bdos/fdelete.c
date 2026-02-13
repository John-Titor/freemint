#include "mintboot/mb_rom.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_portable.h"

long mb_bdos_fdelete(const char *fn)
{
	mb_panic("Fdelete(fn=%08x, \"%s\")", (uint32_t)(uintptr_t)fn,
		 mb_guarded_str(fn));
	return -1;
}
