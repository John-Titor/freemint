#include "mintboot/mb_linea.h"

#include <stddef.h>

struct mb_linea_screen {
	int16_t hidecnt;
	int16_t mcurx;
	int16_t mcury;
	int8_t mdraw;
	int8_t mouseflag;
	int32_t junk1;
	int16_t savex;
	int16_t savey;
	int16_t msavelen;
	int32_t msaveaddr;
	int16_t msavestat;
	int32_t msavearea[64];
	int32_t user_tim;
	int32_t next_tim;
	int32_t user_but;
	int32_t user_cur;
	int32_t user_mot;
	int16_t cheight;
	int16_t maxx;
	int16_t maxy;
	int16_t linelen;
	int16_t bgcol;
	int16_t fgcol;
	uint8_t *cursaddr;
	int16_t v_cur_of;
	int16_t cx;
	int16_t cy;
	int8_t period;
	int8_t curstimer;
	uint8_t *fontdata;
	int16_t firstcode;
	int16_t lastcode;
	int16_t form_width;
	int16_t xpixel;
	uint8_t *fontoff;
	int8_t flags;
	int8_t v_delay;
	int16_t ypixel;
	int16_t width;
	int16_t planes;
	int16_t planesiz;
};

typedef char mb_linea_planes_offset_must_match[
	(offsetof(struct mb_linea_screen, planes) == 346u) ? 1 : -1];

static struct mb_linea_screen mb_linea_screen;

void mb_linea_init(void)
{
	mb_linea_screen.planes = 1;
	mb_linea_screen.planesiz = 0;
}

uint32_t mb_linea_struct_ptr(void)
{
	return (uint32_t)(uintptr_t)&mb_linea_screen.planes;
}
