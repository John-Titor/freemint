#include "mb_fat_tests_internal.h"

void mb_fat_tests_phase_lookup_io(struct mb_fat_test_ctx *t)
{
	long rc;
	long rc2;
	long fh;
	long n;
	int found = 0;
	int found_inner = 0;
	int i;
	uint32_t free_bytes;
	uint32_t total_bytes;
	const char *bad_drive = "1:\\HELLO.TXT";
	const char expect[] = "mintboot FAT16 test file\n";
	const char expect_inner[] = "mintboot nested file\n";

	rc = mb_tests_fsfirst(t->spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(t->dta.dta_name, "HELLO.TXT") == 0) {
			found = 1;
			break;
		}
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc == 0)
		mb_tests_drain_search();

	if (!found)
		mb_panic("FAT test: HELLO.TXT not found");

	fh = Fopen(t->ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT failed");

	n = Fread((uint16_t)fh, sizeof(t->buf), t->buf);
	Fclose((uint16_t)fh);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++) {
		t->handles[i] = Fopen(t->ddelete_file, 0);
		if (t->handles[i] < 0)
			mb_panic("FAT test: Fopen bulk rc=%d at %d",
				 (int)t->handles[i], i);
	}
	fh = Fopen(t->ddelete_file, 0);
	if (fh != MB_ERR_NHNDL)
		mb_panic("FAT test: Fopen ENHNDL rc=%d expected %d", (int)fh,
			 MB_ERR_NHNDL);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++)
		Fclose((uint16_t)t->handles[i]);
	if (n != (long)(sizeof(expect) - 1))
		mb_panic("FAT test: Fread size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect) - 1));
	if (mb_tests_memcmp(t->buf, (const uint8_t *)expect,
			    (uint32_t)(sizeof(expect) - 1)) != 0)
		mb_panic("FAT test: HELLO.TXT contents mismatch");

	rc = Fclose(1);
	if (rc != MB_ERR_IHNDL)
		mb_panic("FAT test: Fclose badf rc=%d expected %d", (int)rc,
			 MB_ERR_IHNDL);

	rc = Fread(1, sizeof(t->buf), t->buf);
	if (rc != MB_ERR_IHNDL)
		mb_panic("FAT test: Fread badf rc=%d expected %d", (int)rc,
			 MB_ERR_IHNDL);

	fh = Fopen(t->ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT for lock failed");
	Fclose((uint16_t)fh);

	fh = Fopen(t->missing, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen missing rc=%d expected %d", (int)fh,
			 MB_ERR_FILNF);

	fh = Fopen(t->missing_dir, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen missing dir rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	Fsetdta(&t->dta);
	rc = Fsfirst(t->missing_dir_spec, 0x17);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Fsfirst missing dir rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(t->spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst repeat rc=%d", (int)rc);
	for (;;) {
		rc2 = Fsnext();
		if (rc2 != 0)
			break;
	}
	if (rc2 != MB_ERR_NMFIL)
		mb_panic("FAT test: Fsnext end rc=%d expected %d", (int)rc2,
			 MB_ERR_NMFIL);

	rc = mb_tests_fsfirst(t->inner_spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst inner rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(t->dta.dta_name, "INNER.TXT") == 0) {
			found_inner = 1;
			break;
		}
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc == 0)
		mb_tests_drain_search();
	if (!found_inner)
		mb_panic("FAT test: INNER.TXT not found");

	fh = Fopen(t->inner_path, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen INNER.TXT failed");
	n = Fread((uint16_t)fh, sizeof(t->buf), t->buf);
	Fclose((uint16_t)fh);
	if (n != (long)(sizeof(expect_inner) - 1))
		mb_panic("FAT test: INNER.TXT size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect_inner) - 1));
	if (mb_tests_memcmp(t->buf, (const uint8_t *)expect_inner,
			    (uint32_t)(sizeof(expect_inner) - 1)) != 0)
		mb_panic("FAT test: INNER.TXT contents mismatch");

	fh = Fopen(t->inner_missing, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen inner missing rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(t->wrong_drive, 0x17, &t->dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Fsfirst wrong drive rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	fh = Fopen(t->wrong_drive, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen wrong drive rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(bad_drive, 0x17, &t->dta);
	if (rc != MB_ERR_DRIVE)
		mb_panic("FAT test: Fsfirst bad drive rc=%d expected %d",
			 (int)rc, MB_ERR_DRIVE);

	fh = Fopen(bad_drive, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen bad drive rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	if (Dfree(t->dfree, mb_tests_drive_dev) != 0)
		mb_panic("FAT test: Dfree failed");

	free_bytes = (uint32_t)t->dfree[0] * t->dfree[2] * t->dfree[3];
	total_bytes = (uint32_t)t->dfree[1] * t->dfree[2] * t->dfree[3];
	if (free_bytes == 0 || free_bytes >= total_bytes)
		mb_panic("FAT test: Dfree free=%u total=%u", free_bytes,
			 total_bytes);

	rc = mb_tests_fsfirst(t->rename_src, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst rename src rc=%d", (int)rc);
	mb_tests_drain_search();
	fh = Fopen(t->rename_src, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen rename src rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(t->ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT for fdatime rc=%d", (int)fh);
	if (Fdatime((uint32_t)(uintptr_t)t->timebuf, (uint16_t)fh, 0) != 0)
		mb_panic("FAT test: Fdatime failed");
	if (Fdatime((uint32_t)(uintptr_t)t->timebuf2, (uint16_t)fh, 0) != 0)
		mb_panic("FAT test: Fdatime repeat failed");
	Fclose((uint16_t)fh);
	if ((t->timebuf[0] == 0 && t->timebuf[1] == 0) ||
	    t->timebuf[0] != t->timebuf2[0] || t->timebuf[1] != t->timebuf2[1])
		mb_panic("FAT test: Fdatime mismatch");
}
