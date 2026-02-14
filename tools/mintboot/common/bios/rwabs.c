#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"

long mb_bios_rwabs(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno,
		  uint16_t dev)
{
	if (dev >= MB_MAX_DRIVES)
		return MB_ERR_DRIVE;

	(void)rwflag;
	(void)buf;
	(void)count;
	(void)recno;
	(void)dev;
	return -1;
}
