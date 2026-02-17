#include "mintboot/mb_rom.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_lowmem.h"

long mb_bios_setexc(uint16_t vnum, uint32_t vptr)
{
	volatile uint32_t *vectors;
	uint32_t prev;

	/* note that Setexc is also used to read/write soft/BDOS vectors */

	if ((vnum >= 263) || (vnum < 2))
		return 0;

	vectors = (volatile uint32_t *)mb_common_vector_base();
	prev = vectors[vnum];

	if (vptr != 0xffffffffu) {
		vectors[vnum] = vptr;
	}
	return (long)prev;
}
