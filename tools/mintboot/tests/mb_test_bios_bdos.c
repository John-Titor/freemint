#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_errors.h"
#include "mb_tests_internal.h"

#include <string.h>

long mb_bios_dispatch(uint16_t fnum, uint16_t *args);
long mb_bdos_dispatch(uint16_t fnum, uint16_t *args);

void mb_tests_bios_bdos(void)
{
	uint16_t drive;
	long drvmap;
	long rc;
	uint8_t sector_a[512];
	uint8_t sector_b[512];
	uint32_t membot;
	uint32_t memtop;
	uint32_t avail;
	uint32_t old_membot;
	uint32_t physbase;
	uint16_t args[4] = { 0, 0, 0, 0 };

	drive = (uint16_t)Dgetdrv();
	drvmap = Drvmap();
	if (drive >= 26)
		mb_panic("BIOS/BDOS test: current drive=%u", (uint32_t)drive);
	if ((drvmap & (1L << drive)) == 0)
		mb_panic("BIOS/BDOS test: drvmap=%08x drive=%u",
			 (uint32_t)drvmap, (uint32_t)drive);

	if ((long)(uintptr_t)Getbpb(drive) <= 0)
		mb_panic("BIOS/BDOS test: Getbpb drive=%u", (uint32_t)drive);

	if (Bcostat(2) != -1)
		mb_panic("BIOS/BDOS test: Bcostat(2)");
	if (Bcostat(0) != 0)
		mb_panic("BIOS/BDOS test: Bcostat(0)");

	rc = Bconstat(2);
	if (rc != 0 && rc != 1)
		mb_panic("BIOS/BDOS test: Bconstat rc=%d", (int)rc);

	if (Bconout(2, 'X') != 0)
		mb_panic("BIOS/BDOS test: Bconout");

	if (Kbshift(0) != 0)
		mb_panic("BIOS/BDOS test: Kbshift");
	physbase = (uint32_t)(uintptr_t)Physbase();
	if (physbase != *mb_lm_v_bas_ad())
		mb_panic("BIOS/BDOS test: Physbase got=%08x exp=%08x",
			 physbase,
			 *mb_lm_v_bas_ad());

	memset(sector_a, 0, sizeof(sector_a));
	memset(sector_b, 0, sizeof(sector_b));
	if (Rwabs(0, sector_a, 1, 1, drive) != 0)
		mb_panic("BIOS/BDOS test: Rwabs read");
	memcpy(sector_b, sector_a, sizeof(sector_a));
	if (Rwabs(1, sector_b, 1, 1, drive) != 0)
		mb_panic("BIOS/BDOS test: Rwabs write");
	memset(sector_b, 0, sizeof(sector_b));
	if (Rwabs(0, sector_b, 1, 1, drive) != 0)
		mb_panic("BIOS/BDOS test: Rwabs reread");
	if (memcmp(sector_a, sector_b, sizeof(sector_a)) != 0)
		mb_panic("BIOS/BDOS test: Rwabs roundtrip mismatch");

	if (Flock(3, 0, 0, 0) != MB_ERR_INVFN)
		mb_panic("BIOS/BDOS test: Flock");

	old_membot = *mb_lm_membot();
	membot = old_membot;
	memtop = *mb_lm_memtop();
	avail = (memtop > membot) ? (memtop - membot) : 0;
	if ((uint32_t)Mxalloc(-1, 0) != avail)
		mb_panic("BIOS/BDOS test: Mxalloc avail");
	if (Mxalloc(-1, 1) != 0 || Mxalloc(-1, 3) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc mode query");
	if (Mxalloc(0, 0) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc zero");
	if (Mxalloc(4, 0) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc alloc");
	if (*mb_lm_membot() != old_membot + 4)
		mb_panic("BIOS/BDOS test: Mxalloc membot");
	*mb_lm_membot() = old_membot;
	if (avail > 0x100u &&
	    Mxalloc((int32_t)(avail + 4u), 0) != MB_ERR_NHNDL)
		mb_panic("BIOS/BDOS test: Mxalloc ENHNDL");

	if (mb_bios_dispatch(0xffffu, args) != MB_ERR_INVFN)
		mb_panic("BIOS/BDOS test: bios dispatch default");
	if (mb_bdos_dispatch(0xffffu, args) != MB_ERR_INVFN)
		mb_panic("BIOS/BDOS test: bdos dispatch default");
}
