#include "mintboot/mb_rom.h"
#include "mintboot/mb_common.h"

long mb_bios_setexc(uint16_t vnum, uint32_t vptr)
{
	volatile uint32_t *vectors;
	uint32_t prev;
	uint32_t base;

	if ((vnum >= 256) || (vnum < 2))
		return 0;

	base = mb_common_vector_base();
	vectors = (volatile uint32_t *)(uintptr_t)base;
	prev = vectors[vnum];

	if (vptr != 0xffffffffu)
		vectors[vnum] = vptr;

	return (long)prev;
}
