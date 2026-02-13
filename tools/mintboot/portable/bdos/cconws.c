#include "mintboot/mb_rom.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_portable.h"

long mb_bdos_cconws(const char *buf)
{
	const char *s = mb_guarded_str(buf);

	mb_log_puts(s);
	return 0;
}
