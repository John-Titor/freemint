#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_errors.h"

long mb_bdos_mxalloc(int32_t amount, uint16_t mode)
{
	uint32_t membot = *mb_lm_membot();
	uint32_t memtop = *mb_lm_memtop();
	uint32_t avail = 0;
	uint32_t alloc;

	if (memtop > membot)
		avail = memtop - membot;

	if (amount < 0) {
		if (mode == 1 || mode == 3)
			return 0;
		return (long)avail;
	}

	if (amount == 0)
		return 0;
	alloc = (uint32_t)amount;
	alloc = (alloc + 3u) & ~3u;
	if (alloc > avail)
		return MB_ERR_NHNDL;
	*mb_lm_membot() = membot + alloc;
	return 0;
}
