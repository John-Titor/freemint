#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_errors.h"
#include "mb_tests_internal.h"

void mb_tests_drive_path(void)
{
	char buf[256];
	long rc;
	uint16_t dev;
	uint32_t map;
	uint16_t invalid_drive = 0xffffu;
	const char *long_rel;
	char longbuf[300];
	uint32_t i;

	dev = mb_common_boot_drive();
	if (dev >= 26)
		mb_panic("Drive tests: invalid boot drive");
	map = (uint32_t)mb_rom_dispatch.drvmap();
	for (i = 0; i < 26; i++) {
		if ((map & (1u << i)) == 0) {
			invalid_drive = (uint16_t)i;
			break;
		}
	}

	rc = Dgetdrv();
	if (rc != (long)dev)
		mb_panic("Dgetdrv: initial=%d expected %d", (int)rc, (int)dev);

	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath(0) rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath(0) missing root");

	if (invalid_drive != 0xffffu) {
		rc = Dsetdrv(invalid_drive);
		if (rc != (long)map)
			mb_panic("Dsetdrv invalid rc=%d", (int)rc);
		if (Dgetdrv() != (long)dev)
			mb_panic("Dsetdrv invalid changed drive");
	}

	rc = Dsetdrv(dev);
	if (rc != (long)mb_rom_dispatch.drvmap())
		mb_panic("Dsetdrv current rc=%d", (int)rc);
	if (Dgetdrv() != (long)dev)
		mb_panic("Dsetdrv current mismatch");

	rc = Dsetpath("C:");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);
	rc = Dsetpath("C:FOO");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);
	rc = Dsetpath("C:\\MINT");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Dsetpath("no_such_dir");
	if (rc == 0)
		mb_panic("Dsetpath missing dir accepted");

	rc = Dsetpath("\\");
	if (rc != 0)
		mb_panic("Dsetpath root rc=%d", (int)rc);

	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath(0) after root rc=%d", (int)rc);
	if (buf[0] != '\\' || buf[1] != '\0')
		mb_panic("Dgetpath root mismatch");

	rc = Dsetpath("MINT");
	if (rc != 0)
		mb_panic("Dsetpath rel rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath after rel rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath rel missing root");

	rc = Dsetpath("\\MINT");
	if (rc != 0)
		mb_panic("Dsetpath abs rc=%d", (int)rc);

	rc = Dsetpath("1-19-cur");
	if (rc != 0)
		mb_panic("Dsetpath rel2 rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath rel2 rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath rel2 missing root");

	long_rel = "\\MINT\\1-19-cur";
	rc = Dsetpath(long_rel);
	if (rc != 0)
		mb_panic("Dsetpath long rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath long rc=%d", (int)rc);

	for (i = 0; i < sizeof(longbuf) - 1; i++)
		longbuf[i] = 'A';
	longbuf[sizeof(longbuf) - 1] = '\0';
	rc = Dsetpath(longbuf);
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath long rc=%d expected %d", (int)rc, MB_ERR_PTHNF);

	rc = Dgetpath(buf, (uint16_t)(dev + 1u));
	if (rc != 0)
		mb_panic("Dgetpath current rc=%d", (int)rc);

	rc = Dgetpath(buf, 27);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Dgetpath bad drive rc=%d", (int)rc);
}
