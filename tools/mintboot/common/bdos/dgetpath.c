#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"

#include <stddef.h>

long mb_bdos_dgetpath(char *buf, uint16_t drive)
{
	const char *path;
	size_t i = 0;

	if (!buf)
		return MB_ERR_PTHNF;
	if (drive == 0)
		drive = mb_bdos_get_current_drive();
	else
		drive = (uint16_t)(drive - 1);
	if (drive >= 26)
		return MB_ERR_DRIVE;

	path = mb_bdos_get_current_path(drive);
	while (path[i] && i + 1 < 256) {
		buf[i] = path[i];
		i++;
	}
	buf[i] = '\0';
	return 0;
}
