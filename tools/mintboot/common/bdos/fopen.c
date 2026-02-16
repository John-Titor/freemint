#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_common.h"

long mb_bdos_fopen(const char *filename, uint16_t mode)
{
	long rc = mb_fat_fopen(filename, mode);
	return rc;
}
