#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"

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
	int found_inner = 0;

	mb_log_puts("mintboot: FAT test start\r\n");
	mb_rom_fsetdta(&dta);
	rc = mb_rom_fsfirst(spec, 0x17);
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

	if (!found)
		mb_panic("FAT test: HELLO.TXT not found");

	fh = mb_rom_fopen("C:\\HELLO.TXT", 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT failed");

	n = mb_rom_fread((uint16_t)fh, sizeof(buf),
			 (uint32_t)(uintptr_t)buf);
	mb_rom_fclose((uint16_t)fh);
	if (n != (long)(sizeof(expect) - 1))
		mb_panic("FAT test: Fread size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect,
			    (uint32_t)(sizeof(expect) - 1)) != 0)
		mb_panic("FAT test: HELLO.TXT contents mismatch");

	fh = mb_rom_fopen(missing, 0);
	if (fh != -33)
		mb_panic("FAT test: Fopen missing rc=%ld expected -33", fh);

	fh = mb_rom_fopen(missing_dir, 0);
	if (fh != -33)
		mb_panic("FAT test: Fopen missing dir rc=%ld expected -33", fh);

	mb_rom_fsetdta(&dta);
	rc = mb_rom_fsfirst(missing_dir_spec, 0x17);
	if (rc != -33)
		mb_panic("FAT test: Fsfirst missing dir rc=%d expected -33", (int)rc);

	mb_rom_fsetdta(&dta);
	rc = mb_rom_fsfirst(inner_spec, 0x17);
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
	if (!found_inner)
		mb_panic("FAT test: INNER.TXT not found");

	fh = mb_rom_fopen(inner_path, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen INNER.TXT failed");
	n = mb_rom_fread((uint16_t)fh, sizeof(buf),
			 (uint32_t)(uintptr_t)buf);
	mb_rom_fclose((uint16_t)fh);
	if (n != (long)(sizeof(expect_inner) - 1))
		mb_panic("FAT test: INNER.TXT size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect_inner) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect_inner,
			    (uint32_t)(sizeof(expect_inner) - 1)) != 0)
		mb_panic("FAT test: INNER.TXT contents mismatch");

	fh = mb_rom_fopen(inner_missing, 0);
	if (fh != -33)
		mb_panic("FAT test: Fopen inner missing rc=%ld expected -33", fh);

	mb_rom_fsetdta(&dta);
	rc = mb_rom_fsfirst(wrong_drive, 0x17);
	if (rc != -33)
		mb_panic("FAT test: Fsfirst wrong drive rc=%d expected -33", (int)rc);

	fh = mb_rom_fopen(wrong_drive, 0);
	if (fh != -33)
		mb_panic("FAT test: Fopen wrong drive rc=%ld expected -33", fh);

	mb_log_puts("mintboot: FAT test done\r\n");
}
