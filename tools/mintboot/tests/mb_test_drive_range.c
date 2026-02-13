#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_errors.h"
#include "mb_tests_internal.h"

void mb_tests_drive_range(void)
{
	uint8_t sector[512];
	long rc;

	rc = Rwabs(0, sector, 1, 0, 26);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Rwabs drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = (long)(uintptr_t)Getbpb(26);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Getbpb drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = Dfree(sector, 26);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Dfree drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);
}
