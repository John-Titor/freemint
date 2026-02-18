#ifndef MINTBOOT_MB_BOARD_H
#define MINTBOOT_MB_BOARD_H

#include "mintboot/mb_common.h"

/* Board-specific early init (clocks, memory, console). */
void mb_board_early_init(void);
void mb_board_init(void);

/* Board hook to override cookie defaults after common init. */
void mb_board_init_cookies(void);

/* Board-specific console I/O used by common layer. */
void mb_board_console_putc(int ch);
void mb_board_exit(int code) NORETURN;

#endif /* MINTBOOT_MB_BOARD_H */
