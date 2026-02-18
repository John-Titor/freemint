#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_errors.h"
#include "mb_tests_internal.h"

#include "mintboot/mb_lib.h"

void mb_tests_bios_bdos(void)
{
	typedef long (*mb_xcon_lowmem_fn)(uint32_t);
	typedef long (*mb_mediach_lowmem_fn)(void);
	uint16_t drive;
	long drvmap;
	long rc;
	uint8_t sector_a[512];
	uint8_t sector_b[512];
	uint32_t membot;
	uint32_t memtop;
	uint32_t avail;
	uint32_t alloc_ptr;
	uint32_t physbase;
	void *saved_dta;
	uint8_t dta_buf[44];
	volatile uint32_t *xconstat;
	volatile uint32_t *xcostat;
	volatile uint32_t *xconout;
	mb_xcon_lowmem_fn xconstat_fn;
	mb_xcon_lowmem_fn xcostat_fn;
	mb_xcon_lowmem_fn xconout_fn;
	mb_mediach_lowmem_fn mediach_fn;
	uint16_t args[4] = { 0, 0, 0, 0 };

	drive = (uint16_t)Dgetdrv();
	drvmap = Drvmap();
	if (drive >= MB_MAX_DRIVES)
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

	if (Cconws("mintboot Cconws smoke\r") != 0)
		mb_panic("BIOS/BDOS test: Cconws");

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

	saved_dta = mb_bdos_fgetdta();
	if (saved_dta == NULL)
		mb_panic("BIOS/BDOS test: Fgetdta null");
	if (mb_bdos_fsetdta(dta_buf) != 0)
		mb_panic("BIOS/BDOS test: Fsetdta set");
	if (mb_bdos_fgetdta() != dta_buf)
		mb_panic("BIOS/BDOS test: Fsetdta mismatch");
	if (mb_bdos_fsetdta(saved_dta) != 0)
		mb_panic("BIOS/BDOS test: Fsetdta restore");
	if (mb_bdos_fgetdta() != saved_dta)
		mb_panic("BIOS/BDOS test: Fsetdta restore mismatch");

	xconstat = mb_lm_xconstat();
	xcostat = mb_lm_xcostat();
	xconout = mb_lm_xconout();
	if (xconstat[0] == 0 || xcostat[0] == 0 || xconout[0] == 0)
		mb_panic("BIOS/BDOS test: xcon lowmem vectors missing");

	xconstat_fn = (mb_xcon_lowmem_fn)(uintptr_t)xconstat[0];
	xcostat_fn = (mb_xcon_lowmem_fn)(uintptr_t)xcostat[0];
	xconout_fn = (mb_xcon_lowmem_fn)(uintptr_t)xconout[0];
	rc = xconstat_fn((uint32_t)(2u << 16));
	if (rc != 0 && rc != 1)
		mb_panic("BIOS/BDOS test: xconstat rc=%d", (int)rc);
	if (xcostat_fn((uint32_t)(2u << 16)) != -1)
		mb_panic("BIOS/BDOS test: xcostat(2)");
	if (xcostat_fn(0) != 0)
		mb_panic("BIOS/BDOS test: xcostat(0)");
	if (xconout_fn((uint32_t)((2u << 16) | 'Y')) != 0)
		mb_panic("BIOS/BDOS test: xconout");

	if (*mb_lm_hdv_mediach() == 0)
		mb_panic("BIOS/BDOS test: hdv_mediach vector missing");
	mediach_fn = (mb_mediach_lowmem_fn)(uintptr_t)(*mb_lm_hdv_mediach());
	if (mediach_fn() != 0)
		mb_panic("BIOS/BDOS test: hdv_mediach");

	membot = *mb_lm_membot();
	memtop = *mb_lm_memtop();
	avail = (memtop > membot) ? (memtop - membot) : 0;
	if ((uint32_t)Malloc(-1) != (uint32_t)Mxalloc(-1, 0))
		mb_panic("BIOS/BDOS test: Malloc/Mxalloc avail");
	if ((uint32_t)Mxalloc(-1, 0) == 0 || (uint32_t)Mxalloc(-1, 0) > avail)
		mb_panic("BIOS/BDOS test: Mxalloc avail");
	if (Mxalloc(-1, 1) != 0 || (uint32_t)Mxalloc(-1, 3) != (uint32_t)Mxalloc(-1, 0))
		mb_panic("BIOS/BDOS test: Mxalloc mode query");
	if (Malloc(0) != 0)
		mb_panic("BIOS/BDOS test: Malloc zero");
	if (Mxalloc(0, 0) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc zero");
	alloc_ptr = (uint32_t)Mxalloc(16, 0);
	if (alloc_ptr == 0 || (alloc_ptr & 3u) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc alloc");
	if (Mshrink((void *)(uintptr_t)alloc_ptr, 8) != 0)
		mb_panic("BIOS/BDOS test: Mshrink");
	if (Mfree((void *)(uintptr_t)alloc_ptr) != 0)
		mb_panic("BIOS/BDOS test: Mfree");
	if (Mxalloc((int32_t)(avail + 4u), 0) != 0)
		mb_panic("BIOS/BDOS test: Mxalloc fail");

	if (mb_bios_dispatch(0xffffu, args, NULL) != MB_ERR_INVFN)
		mb_panic("BIOS/BDOS test: bios dispatch default");
	if (mb_bdos_dispatch(0xffffu, args, NULL) != MB_ERR_INVFN)
		mb_panic("BIOS/BDOS test: bdos dispatch default");
}
