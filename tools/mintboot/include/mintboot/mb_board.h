#ifndef MINTBOOT_MB_BOARD_H
#define MINTBOOT_MB_BOARD_H

#include <stdint.h>

struct mb_boot_info;

/* Board-specific early init (clocks, memory, console). */
void mb_board_early_init(void);

/* Load kernel image into memory and populate boot info. */
int mb_board_load_kernel(struct mb_boot_info *info);

/* Board-specific console I/O used by portable layer. */
void mb_board_console_putc(int ch);
int mb_board_console_getc(void);
int mb_board_console_poll(void);
void mb_board_exit(int code);

#endif /* MINTBOOT_MB_BOARD_H */
