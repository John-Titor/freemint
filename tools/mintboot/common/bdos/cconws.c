#include "mintboot/mb_rom.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_common.h"

long mb_bdos_cconws(const char *buf)
{
	mb_log_puts(buf);
	return 0;
}
