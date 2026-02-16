#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_rom.h"

#define MB_XCON_TABLE_SLOTS 8u

static long mb_hdv_bpb_stub(uint16_t dev)
{
	return mb_bios_getbpb(dev);
}

static long mb_hdv_rw_stub(uint16_t rwflag, void *buf, uint16_t count,
			   uint16_t recno, uint16_t dev)
{
	return mb_bios_rwabs(rwflag, buf, count, recno, dev);
}

static long mb_hdv_mediach_stub(uint16_t dev)
{
	(void)dev;
	return 0;
}

static long mb_xconstat_stub(uint16_t dev)
{
	return mb_bios_bconstat(dev);
}

static long mb_xconin_stub(uint16_t dev)
{
	return mb_bios_bconin(dev);
}

static long mb_xcostat_stub(uint16_t dev)
{
	return mb_bios_bcostat(dev);
}

static long mb_xconout_stub(uint16_t dev, uint16_t c)
{
	return mb_bios_bconout(dev, c);
}

void mb_bios_init_lowmem(void)
{
	uint16_t i;

	*mb_lm_hdv_bpb() = (uint32_t)(uintptr_t)mb_hdv_bpb_stub;
	*mb_lm_hdv_rw() = (uint32_t)(uintptr_t)mb_hdv_rw_stub;
	*mb_lm_hdv_mediach() = (uint32_t)(uintptr_t)mb_hdv_mediach_stub;

	for (i = 0; i < MB_XCON_TABLE_SLOTS; i++) {
		mb_lm_xconstat()[i] = (uint32_t)(uintptr_t)mb_xconstat_stub;
		mb_lm_xconin()[i] = (uint32_t)(uintptr_t)mb_xconin_stub;
		mb_lm_xcostat()[i] = (uint32_t)(uintptr_t)mb_xcostat_stub;
		mb_lm_xconout()[i] = (uint32_t)(uintptr_t)mb_xconout_stub;
	}
}
