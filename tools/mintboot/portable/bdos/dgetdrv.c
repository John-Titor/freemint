#include "mintboot/mb_rom.h"

long mb_bdos_dgetdrv(void)
{
	return mb_bdos_get_current_drive();
}
