#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_portable.h"

long mb_bdos_fopen(const char *filename, uint16_t mode)
{
	long rc = mb_fat_fopen(filename, mode);

	mb_log_printf("bdos_fopen('%s',%u) => %d\r\n",
		      mb_guarded_str(filename),
		      (uint32_t)mode, (int32_t)rc);
	return rc;
}
