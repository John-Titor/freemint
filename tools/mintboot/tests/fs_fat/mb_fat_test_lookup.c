#include "mb_fat_tests_internal.h"
#include "mintboot/mb_util.h"

void mb_fat_tests_phase_lookup_io(struct mb_fat_test_ctx *t)
{
	long rc;
	long fh;
	long n;
	int i;
	uint32_t free_bytes;
	uint32_t total_bytes;
	uint8_t wbuf[700];
	uint8_t rbuf[700];
	uint8_t one = 0x5a;
	uint32_t k;
	const char rewrite_one[] = "Z";
	const char overwrite[] = "OVERRIDE-HEADER";
	const uint32_t overwrite_len = (uint32_t)(sizeof(overwrite) - 1);
	const char *bad_drive = "1:\\HELLO.TXT";
	const char expect[] = "mintboot FAT16 test file\n";
	const char expect_inner[] = "mintboot nested file\n";

	if (t->ddelete_file[0] == '\0' || t->ddelete_file[1] != ':' ||
	    t->ddelete_file[2] != '\\')
		mb_panic("FAT test: bad open path '%s'",
			 mb_guarded_str(t->ddelete_file));

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

	fh = Fopen(t->wrong_drive, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen wrong drive rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

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

	fh = Fcreate(t->fcreate_missing_parent, 0);
	if (fh != MB_ERR_PTHNF)
		mb_panic("FAT test: Fcreate missing parent rc=%d expected %d",
			 (int)fh, MB_ERR_PTHNF);

	fh = Fcreate(t->fcreate_dir_target, 0);
	if (fh != MB_ERR_ACCDN)
		mb_panic("FAT test: Fcreate dir target rc=%d expected %d",
			 (int)fh, MB_ERR_ACCDN);

	fh = Fcreate(t->fcreate_new, 0);
	if (fh < 0)
		mb_panic("FAT test: Fcreate new rc=%d", (int)fh);
	for (k = 0; k < (uint32_t)sizeof(wbuf); k++)
		wbuf[k] = (uint8_t)(k & 0xffu);
	n = Fwrite((uint16_t)fh, (uint32_t)sizeof(wbuf), wbuf);
	if (n != (long)sizeof(wbuf))
		mb_panic("FAT test: Fwrite full rc=%d", (int)n);
	n = Fseek(0, (uint16_t)fh, 0);
	if (n != 0)
		mb_panic("FAT test: Fseek rewrite rc=%d", (int)n);
	n = Fwrite((uint16_t)fh, overwrite_len, (void *)overwrite);
	if (n != (long)overwrite_len)
		mb_panic("FAT test: Fwrite overwrite rc=%d", (int)n);
	if (Fclose((uint16_t)fh) != 0)
		mb_panic("FAT test: Fclose create");

	fh = Fopen(t->fcreate_new, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen created rc=%d", (int)fh);
	n = Fread((uint16_t)fh, (uint32_t)sizeof(rbuf), rbuf);
	if (n != (long)sizeof(rbuf))
		mb_panic("FAT test: Fread created size rc=%d", (int)n);
	if (mb_tests_memcmp(rbuf, (const uint8_t *)overwrite, overwrite_len) != 0)
		mb_panic("FAT test: Fwrite overwrite mismatch");
	if (mb_tests_memcmp(&rbuf[overwrite_len], &wbuf[overwrite_len],
			    (uint32_t)sizeof(rbuf) - overwrite_len) != 0)
		mb_panic("FAT test: Fwrite payload mismatch");
	n = Fwrite((uint16_t)fh, 1, &one);
	if (n != MB_ERR_ACCDN)
		mb_panic("FAT test: Fwrite readonly handle rc=%d expected %d",
			 (int)n, MB_ERR_ACCDN);
	if (Fclose((uint16_t)fh) != 0)
		mb_panic("FAT test: Fclose created");

	n = Fwrite(1, 1, &one);
	if (n != MB_ERR_IHNDL)
		mb_panic("FAT test: Fwrite badf rc=%d expected %d",
			 (int)n, MB_ERR_IHNDL);

	fh = Fcreate(t->fcreate_rewrite, 0);
	if (fh < 0)
		mb_panic("FAT test: Fcreate rewrite rc=%d", (int)fh);
	n = Fwrite((uint16_t)fh, 1, (void *)rewrite_one);
	if (n != 1)
		mb_panic("FAT test: Fwrite rewrite rc=%d", (int)n);
	if (Fclose((uint16_t)fh) != 0)
		mb_panic("FAT test: Fclose rewrite");
	fh = Fopen(t->fcreate_rewrite, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen rewrite rc=%d", (int)fh);
	n = Fread((uint16_t)fh, sizeof(t->buf), t->buf);
	if (n != 1 || t->buf[0] != (uint8_t)rewrite_one[0])
		mb_panic("FAT test: Fcreate truncate/read rc=%d ch=%u", (int)n,
			 (uint32_t)t->buf[0]);
	if (Fclose((uint16_t)fh) != 0)
		mb_panic("FAT test: Fclose rewrite read");

	n = Fdelete(t->fdelete_missing_file);
	if (n != MB_ERR_FILNF)
		mb_panic("FAT test: Fdelete missing rc=%d expected %d",
			 (int)n, MB_ERR_FILNF);

	n = Fdelete(t->fdelete_dir_target);
	if (n != MB_ERR_ACCDN)
		mb_panic("FAT test: Fdelete dir rc=%d expected %d",
			 (int)n, MB_ERR_ACCDN);

	mb_tests_set_readonly("REWRITE TXT");
	n = Fdelete(t->fcreate_rewrite);
	if (n != MB_ERR_ACCDN)
		mb_panic("FAT test: Fdelete readonly rc=%d expected %d",
			 (int)n, MB_ERR_ACCDN);

	n = Fdelete(t->fcreate_new);
	if (n != 0)
		mb_panic("FAT test: Fdelete new rc=%d", (int)n);
	fh = Fopen(t->fcreate_new, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen deleted rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);
}
