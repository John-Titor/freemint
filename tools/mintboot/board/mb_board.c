#include "mintboot/mb_portable.h"
#include "mintboot/mb_board.h"

void mb_board_early_init(void)
{
	/* TODO: hardware init for target board. */
}

int mb_board_load_kernel(struct mb_boot_info *info)
{
	(void)info;
	/* TODO: load kernel image, set entry/base/size in info. */
	return -1;
}

void mb_board_console_putc(int ch)
{
	(void)ch;
	/* TODO: output character to board console. */
}

int mb_board_console_getc(void)
{
	/* TODO: blocking read. */
	return -1;
}

int mb_board_console_poll(void)
{
	/* TODO: return non-zero if input available. */
	return 0;
}

void mb_board_exit(int code)
{
	(void)code;
	/* TODO: board-specific shutdown. */
	for (;;) {
	}
}
