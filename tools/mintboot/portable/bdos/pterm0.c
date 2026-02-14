#include "mintboot/mb_rom.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_portable.h"

long mb_bdos_pterm0(void)
{
	mb_log_puts("Pterm0\n");
	mb_board_exit(0);
	return 0;
}
