#include "mintboot/mb_rom.h"
#include "mintboot/mb_xbios_iorec.h"

#include <stddef.h>

static long mb_bconmap_bconstat(void)
{
	return mb_rom_dispatch.bconstat(6);
}

static long mb_bconmap_bconin(void)
{
	return mb_rom_dispatch.bconin(6);
}

static long mb_bconmap_bcostat(void)
{
	return mb_rom_dispatch.bcostat(6);
}

static long mb_bconmap_bconout(uint16_t dev, uint16_t c)
{
	(void)dev;
	return mb_rom_dispatch.bconout(6, c);
}

static long mb_bconmap_rsconf(uint16_t baud, uint16_t flow, uint16_t uc,
			      uint16_t rs, uint16_t ts, uint16_t sc)
{
	return mb_xbios_rsconf(baud, flow, uc, rs, ts, sc);
}

static struct mb_bconmap *mb_bconmap_root(void)
{
	static struct mb_maptab map = {
		.bconstat = mb_bconmap_bconstat,
		.bconin = mb_bconmap_bconin,
		.bcostat = mb_bconmap_bcostat,
		.bconout = mb_bconmap_bconout,
		.rsconf = mb_bconmap_rsconf,
		.iorec = NULL,
	};
	static struct mb_bconmap root = {
		.maptab = &map,
		.maptabsize = 1,
		.mapped_device = 6,
	};

	if (!map.iorec)
		map.iorec = mb_xbios_iorec_get();

	return &root;
}

long mb_xbios_bconmap(uint16_t dev)
{
	struct mb_bconmap *root = mb_bconmap_root();
	uint16_t old_dev = root->mapped_device;

	if (dev == 0xffffu)
		return old_dev;

	if (dev == 0xfffeu)
		return (long)(uintptr_t)root;

	if (dev != 6)
		return 0;

	root->mapped_device = 6;
	return old_dev;
}
