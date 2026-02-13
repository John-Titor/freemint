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

struct mb_maptab {
	long (*bconstat)(void);
	long (*bconin)(void);
	long (*bcostat)(void);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rsconf)(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs,
		       uint16_t ts, uint16_t sc);
	struct mb_ext_iorec *iorec;
} __attribute__((packed));

struct mb_bconmap {
	struct mb_maptab *maptab;
	uint16_t maptabsize;
	uint16_t mapped_device;
} __attribute__((packed));

struct mb_ext_iorec *mb_xbios_iorec_get(void);

#endif /* MINTBOOT_MB_XBIOS_IOREC_H */
