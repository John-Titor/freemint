#include "mintboot/mb_rom.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_lowmem.h"

extern uint8_t _mb_image_start[] __attribute__((weak));
extern uint8_t _mb_image_end[] __attribute__((weak));

static int mb_setexc_target_sane(uint32_t vptr)
{
	uint32_t st_lo = *mb_lm_membot();
	uint32_t st_hi = *mb_lm_phystop();
	uint32_t mb_lo;
	uint32_t mb_hi;
	int in_st_ram = 0;
	int in_mintboot = 0;

	if ((vptr & 1u) != 0u)
		return 0;

	if (_mb_image_start && _mb_image_end) {
		mb_lo = (uint32_t)(uintptr_t)_mb_image_start;
		mb_hi = (uint32_t)(uintptr_t)_mb_image_end;
	} else {
		mb_lo = (uint32_t)(uintptr_t)mb_common_set_st_ram;
		mb_hi = mb_lo;
	}

	if (st_hi > st_lo)
		in_st_ram = (vptr >= st_lo) && (vptr < st_hi);
	if (mb_hi > mb_lo)
		in_mintboot = (vptr >= mb_lo) && (vptr < mb_hi);

	return in_st_ram || in_mintboot;
}

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

	if (vptr != 0xffffffffu) {
		if (!mb_setexc_target_sane(vptr)) {
			mb_log_printf("setexc: reject vnum=%u vptr=%08x\n", vnum, vptr);
			return (long)prev;
		}
		vectors[vnum] = vptr;
	}

	return (long)prev;
}
