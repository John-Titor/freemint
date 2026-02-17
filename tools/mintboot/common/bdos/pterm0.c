#include "mintboot/mb_rom.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_common.h"

long mb_bdos_pterm0(void)
{
	mb_panic("kernel called Pterm0");
	return 0;
}
