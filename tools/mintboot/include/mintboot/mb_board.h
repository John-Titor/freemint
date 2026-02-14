#ifndef MINTBOOT_MB_BOARD_H
#define MINTBOOT_MB_BOARD_H

/* Board-specific early init (clocks, memory, console). */
void mb_board_early_init(void);

/* Board hook to override cookie defaults after common init. */
void mb_board_init_cookies(void);

/* Board-specific console I/O used by common layer. */
void mb_board_console_putc(int ch);
int mb_board_console_getc(void);
int mb_board_console_poll(void);
void mb_board_exit(int code);

#endif /* MINTBOOT_MB_BOARD_H */
