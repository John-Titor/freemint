#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

struct mb_test_dta {
	uint16_t index;
	uint32_t magic;
	char dta_pat[14];
	char dta_sattrib;
	char dta_attrib;
	uint16_t dta_time;
	uint16_t dta_date;
	uint32_t dta_size;
	char dta_name[14];
};

static int mb_tests_strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

static int mb_tests_memcmp(const uint8_t *a, const uint8_t *b, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		if (a[i] != b[i])
			return (int)a[i] - (int)b[i];
	}
	return 0;
}

static long mb_tests_fsfirst(const char *spec, uint16_t attr,
			     struct mb_test_dta *dta)
{
	mb_rom_fsetdta(dta);
	return mb_rom_fsfirst(spec, attr);
}

static void mb_tests_drain_search(void)
{
	while (mb_rom_fsnext() == 0) {
	}
}

void mb_portable_run_tests(void)
{
	char spec[] = "C:\\*.*";
	struct mb_test_dta dta;
	uint8_t buf[128];
	long rc;
	long fh;
	long n;
	int found = 0;
	const char expect[] = "mintboot FAT16 test file\n";
	const char expect_inner[] = "mintboot nested file\n";
	const char *missing = "C:\\NOFILE.TXT";
	const char *missing_dir = "C:\\NOSUCH\\HELLO.TXT";
	const char *missing_dir_spec = "C:\\NOSUCH\\*.*";
	const char *wrong_drive = "D:\\HELLO.TXT";
	const char *inner_path = "C:\\SUBDIR\\INNER.TXT";
	const char *inner_missing = "C:\\SUBDIR\\NOFILE.TXT";
	const char *inner_spec = "C:\\SUBDIR\\*.*";
	const char *rename_src = "C:\\RENAME.TXT";
	const char *rename_dst = "C:\\RENAMED.TXT";
	const char *move_src = "C:\\MOVE.TXT";
	const char *move_dst = "C:\\SUBDIR\\MOVED.TXT";
	const char *rename_dir_src = "C:\\SUBDIR";
	const char *rename_dir_dst = "C:\\NEWDIR";
	const char *missing_rename = "C:\\NOFILE.TXT";
	const char *missing_dir_rename = "C:\\NODIR";
	const char *missing_dst_dir = "C:\\NOSUCH\\RENAMED.TXT";
	const char *wrong_drive_rename = "D:\\RENAMED.TXT";
	const char *renamed_spec = "C:\\RENAMED.TXT";
	const char *moved_spec = "C:\\SUBDIR\\MOVED.TXT";
	const char *newdir_spec = "C:\\NEWDIR";
	int found_inner = 0;
	uint16_t dfree[4];
	uint32_t free_bytes;
	uint32_t total_bytes;
	long rc2;
	int i;
	long handles[MB_FAT_MAX_OPEN];

	mb_log_puts("mintboot: FAT test start\r\n");
	rc = mb_tests_fsfirst(spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "HELLO.TXT") == 0) {
			found = 1;
			break;
		}
		rc = mb_rom_fsnext();
		if (rc != 0)
			break;
	}
	if (rc == 0)
		mb_tests_drain_search();

	if (!found)
		mb_panic("FAT test: HELLO.TXT not found");

	fh = mb_rom_fopen("C:\\HELLO.TXT", 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT failed");

	n = mb_rom_fread((uint16_t)fh, sizeof(buf), buf);
	mb_rom_fclose((uint16_t)fh);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++) {
		handles[i] = mb_rom_fopen("C:\\HELLO.TXT", 0);
		if (handles[i] < 0)
			mb_panic("FAT test: Fopen bulk rc=%ld at %d", handles[i],
				 i);
	}
	fh = mb_rom_fopen("C:\\HELLO.TXT", 0);
	if (fh != MB_ERR_NMFIL)
		mb_panic("FAT test: Fopen ENMFIL rc=%ld expected %d", fh,
			 MB_ERR_NMFIL);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++) {
		mb_rom_fclose((uint16_t)handles[i]);
	}
	if (n != (long)(sizeof(expect) - 1))
		mb_panic("FAT test: Fread size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect,
			    (uint32_t)(sizeof(expect) - 1)) != 0)
		mb_panic("FAT test: HELLO.TXT contents mismatch");

	rc = mb_rom_fclose(1);
	if (rc != MB_ERR_BADF)
		mb_panic("FAT test: Fclose badf rc=%ld expected %d", rc,
			 MB_ERR_BADF);

	rc = mb_rom_fread(1, sizeof(buf), buf);
	if (rc != MB_ERR_BADF)
		mb_panic("FAT test: Fread badf rc=%ld expected %d", rc,
			 MB_ERR_BADF);

	fh = mb_rom_fopen("C:\\HELLO.TXT", 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT for lock failed");
	rc = mb_rom_flock((uint16_t)fh, 1, 0, 0);
	if (rc != 0)
		mb_panic("FAT test: Flock rc=%ld expected 0", rc);
	rc = mb_rom_fread((uint16_t)fh, 1, buf);
	if (rc != MB_ERR_LOCKED)
		mb_panic("FAT test: Fread locked rc=%ld expected %d", rc,
			 MB_ERR_LOCKED);
	rc = mb_rom_flock((uint16_t)fh, 0, 0, 0);
	if (rc != 0)
		mb_panic("FAT test: Funlock rc=%ld expected 0", rc);
	mb_rom_fclose((uint16_t)fh);

	fh = mb_rom_fopen(missing, 0);
	if (fh != MB_ERR_FNF)
		mb_panic("FAT test: Fopen missing rc=%ld expected %d", fh,
			 MB_ERR_FNF);

	fh = mb_rom_fopen(missing_dir, 0);
	if (fh != MB_ERR_FNF)
		mb_panic("FAT test: Fopen missing dir rc=%ld expected %d", fh,
			 MB_ERR_FNF);

	mb_rom_fsetdta(&dta);
	rc = mb_rom_fsfirst(missing_dir_spec, 0x17);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: Fsfirst missing dir rc=%d expected %d",
			 (int)rc, MB_ERR_FNF);

	rc = mb_tests_fsfirst(spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst repeat rc=%d", (int)rc);
	for (;;) {
		rc2 = mb_rom_fsnext();
		if (rc2 != 0)
			break;
	}
	if (rc2 != MB_ERR_NMFIL)
		mb_panic("FAT test: Fsnext end rc=%ld expected %d", rc2,
			 MB_ERR_NMFIL);

	rc = mb_tests_fsfirst(inner_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst inner rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "INNER.TXT") == 0) {
			found_inner = 1;
			break;
		}
		rc = mb_rom_fsnext();
		if (rc != 0)
			break;
	}
	if (rc == 0)
		mb_tests_drain_search();
	if (!found_inner)
		mb_panic("FAT test: INNER.TXT not found");

	fh = mb_rom_fopen(inner_path, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen INNER.TXT failed");
	n = mb_rom_fread((uint16_t)fh, sizeof(buf), buf);
	mb_rom_fclose((uint16_t)fh);
	if (n != (long)(sizeof(expect_inner) - 1))
		mb_panic("FAT test: INNER.TXT size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect_inner) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect_inner,
			    (uint32_t)(sizeof(expect_inner) - 1)) != 0)
		mb_panic("FAT test: INNER.TXT contents mismatch");

	fh = mb_rom_fopen(inner_missing, 0);
	if (fh != MB_ERR_FNF)
		mb_panic("FAT test: Fopen inner missing rc=%ld expected %d", fh,
			 MB_ERR_FNF);

	rc = mb_tests_fsfirst(wrong_drive, 0x17, &dta);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: Fsfirst wrong drive rc=%d expected %d",
			 (int)rc, MB_ERR_FNF);

	fh = mb_rom_fopen(wrong_drive, 0);
	if (fh != MB_ERR_FNF)
		mb_panic("FAT test: Fopen wrong drive rc=%ld expected %d", fh,
			 MB_ERR_FNF);

	if (mb_rom_dfree((uint32_t)(uintptr_t)dfree, 2) != 0)
		mb_panic("FAT test: Dfree failed");

	free_bytes = (uint32_t)dfree[0] * dfree[2] * dfree[3];
	total_bytes = (uint32_t)dfree[1] * dfree[2] * dfree[3];
	if (free_bytes == 0 || free_bytes >= total_bytes)
		mb_panic("FAT test: Dfree free=%u total=%u", free_bytes,
			 total_bytes);

	rc = mb_tests_fsfirst(rename_src, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst rename src rc=%d", (int)rc);
	mb_tests_drain_search();
	fh = mb_rom_fopen(rename_src, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen rename src rc=%ld", fh);
	mb_rom_fclose((uint16_t)fh);

	rc = mb_rom_frename(0, rename_src, rename_dst);
	if (rc != 0)
		mb_panic("FAT test: Frename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(rename_src, 0x17, &dta);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: old rename rc=%d expected %d", (int)rc,
			 MB_ERR_FNF);
	rc = mb_tests_fsfirst(renamed_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: new rename rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = mb_rom_frename(0, move_src, move_dst);
	if (rc != 0)
		mb_panic("FAT test: move rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(move_src, 0x17, &dta);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: old move rc=%d expected %d", (int)rc,
			 MB_ERR_FNF);
	rc = mb_tests_fsfirst(moved_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: new move rc=%d expected 0", (int)rc);
	mb_tests_drain_search();
	{
		int found_move = 0;
		rc = mb_tests_fsfirst(inner_spec, 0x17, &dta);
		if (rc != 0)
			mb_panic("FAT test: Fsfirst subdir rc=%d", (int)rc);
		for (;;) {
			if (mb_tests_strcmp(dta.dta_name, "MOVED.TXT") == 0) {
				found_move = 1;
				break;
			}
			rc = mb_rom_fsnext();
			if (rc != 0)
				break;
		}
		if (rc == 0)
			mb_tests_drain_search();
		if (!found_move)
			mb_panic("FAT test: MOVED.TXT not found in subdir");
	}

	rc = mb_rom_frename(0, rename_dir_src, rename_dir_dst);
	if (rc != 0)
		mb_panic("FAT test: dir rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(rename_dir_src, 0x17, &dta);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: old dir rc=%d expected %d", (int)rc,
			 MB_ERR_FNF);
	rc = mb_tests_fsfirst(newdir_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: new dir rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = mb_rom_frename(0, rename_dst, wrong_drive_rename);
	if (rc != MB_ERR_XDEV)
		mb_panic("FAT test: xdev rename rc=%d expected %d", (int)rc,
			 MB_ERR_XDEV);

	rc = mb_rom_frename(0, missing_rename, renamed_spec);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: missing file rename rc=%d expected %d",
			 (int)rc, MB_ERR_FNF);

	rc = mb_rom_frename(0, missing_dir_rename, renamed_spec);
	if (rc != MB_ERR_FNF)
		mb_panic("FAT test: missing dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_FNF);

	rc = mb_rom_frename(0, renamed_spec, missing_dst_dir);
	if (rc != MB_ERR_PTH)
		mb_panic("FAT test: missing dst dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_PTH);

	rc = mb_rom_fattrib("C:\\HELLO.TXT", 1, 0);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Fattrib accdn rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	mb_log_puts("mintboot: FAT test done\r\n");
}
