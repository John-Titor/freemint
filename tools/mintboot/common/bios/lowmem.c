#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_rom.h"

#define MB_XCON_TABLE_SLOTS 8u

/*
 * These functions are called directly by the kernel, which uses the -mshort calling convention.
 * Thus, we invoke via the dispatchers in order to get correct argument unpacking.
 */

static long mb_hdv_bpb_stub(uint32_t args)
{
	return mb_bios_dispatch(0x07, (uint16_t *)&args, (uint32_t *)8);
}

static long mb_hdv_rw_stub(uint32_t args)
{
	return mb_bios_dispatch(0x04, (uint16_t *)&args, (uint32_t *)8);
}

static long mb_hdv_mediach_stub(void)
{
	return 0;
}

static long mb_xconstat_stub(uint32_t args)
{
	return mb_bios_dispatch(0x01, (uint16_t *)&args, (uint32_t *)8);
}

static long mb_xconin_stub(uint32_t args)
{
	return mb_bios_dispatch(0x02, (uint16_t *)&args, (uint32_t *)8);
}

static long mb_xcostat_stub(uint32_t args)
{
	return mb_bios_dispatch(0x08, (uint16_t *)&args, (uint32_t *)8);
}

static long mb_xconout_stub(uint32_t args)
{
	return mb_bios_dispatch(0x03, (uint16_t *)&args, (uint32_t *)8);
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
