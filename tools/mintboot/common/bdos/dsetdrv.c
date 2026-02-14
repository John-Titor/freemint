#include "mintboot/mb_rom.h"
#include "mintboot/mb_bdos_state.h"

long mb_bdos_dsetdrv(uint16_t drive)
{
	uint32_t map = (uint32_t)mb_rom_dispatch.drvmap();

	if (drive < MB_MAX_DRIVES && (map & (1u << drive)))
		mb_bdos_set_current_drive(drive);
	return map;
}
