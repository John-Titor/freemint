#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_errors.h"
#include "mb_tests_internal.h"

void mb_tests_drive_range(void)
{
	uint8_t sector[512];
	long rc;
	uint16_t invalid_drive = (uint16_t)MB_MAX_DRIVES;

	rc = Rwabs(0, sector, 1, 0, invalid_drive);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Rwabs drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = (long)(uintptr_t)Getbpb(invalid_drive);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Getbpb drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = Dfree(sector, invalid_drive);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Dfree drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);
}
