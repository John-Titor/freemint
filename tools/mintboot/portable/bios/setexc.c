#include "mintboot/mb_rom.h"
#include "mintboot/mb_portable.h"

long mb_bios_setexc(uint16_t vnum, uint32_t vptr)
{
	volatile uint32_t *vectors;
	uint32_t prev;
	uint32_t base;

	if (vnum >= 256)
		return 0;

	base = mb_portable_vector_base();
	vectors = (volatile uint32_t *)(uintptr_t)base;
	prev = vectors[vnum];

	if (vnum == 11) {
		mb_log_printf("Setexc(vnum=%u, vptr=%08x) prev=%08x\r\n",
			      (uint32_t)vnum, vptr, prev);
	}

	if (vptr != 0xffffffffu)
		vectors[vnum] = vptr;

	return (long)prev;
}
