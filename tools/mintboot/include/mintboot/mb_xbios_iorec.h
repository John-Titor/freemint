#ifndef MINTBOOT_MB_XBIOS_IOREC_H
#define MINTBOOT_MB_XBIOS_IOREC_H

#include <stdint.h>

struct mb_iorec {
	uint8_t *buf;
	uint16_t size;
	volatile uint16_t head;
	volatile uint16_t tail;
	uint16_t low;
	uint16_t high;
} __attribute__((packed));

struct mb_ext_iorec {
	struct mb_iorec in;
	struct mb_iorec out;
	uint8_t baudrate;
	uint8_t flowctrl;
	uint8_t ucr;
	uint8_t datamask;
	uint8_t wr5;
} __attribute__((packed));

struct mb_ext_iorec *mb_xbios_iorec_get(void);
struct mb_iorec *mb_console_iorec_get(void);
int mb_console_iorec_putc(uint8_t ch);
int mb_console_iorec_getc(void);
int mb_console_iorec_ready(void);

#endif /* MINTBOOT_MB_XBIOS_IOREC_H */
