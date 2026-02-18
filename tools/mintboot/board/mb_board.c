#include "mintboot/mb_common.h"
#include "mintboot/mb_board.h"

void mb_board_early_init(void)
{
	/* TODO: hardware init for target board. */
}

void mb_board_init(void)
{
	/* TODO: post-vector init for target board. */
}

void mb_board_console_putc(int ch)
{
	(void)ch;
	/* TODO: output character to board console. */
}

void mb_board_exit(int code)
{
	(void)code;
	/* TODO: board-specific shutdown. */
	for (;;) {
	}
}
