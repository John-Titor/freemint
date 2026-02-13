#include "mb_fat_tests_internal.h"

void mb_fat_tests_phase_rename_attr_seek(struct mb_fat_test_ctx *t)
{
	long rc;
	long fh;

	rc = Frename(0, t->rename_src, t->rename_dst);
	if (rc != 0)
		mb_panic("FAT test: Frename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(t->rename_src, 0x17, &t->dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old rename rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = mb_tests_fsfirst(t->renamed_spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: new rename rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = Frename(0, t->move_src, t->move_dst);
	if (rc != 0)
		mb_panic("FAT test: move rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(t->move_src, 0x17, &t->dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old move rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = mb_tests_fsfirst(t->moved_spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: new move rc=%d expected 0", (int)rc);
	mb_tests_drain_search();
	{
		int found_move = 0;
		rc = mb_tests_fsfirst(t->inner_spec, 0x17, &t->dta);
		if (rc != 0)
			mb_panic("FAT test: Fsfirst subdir rc=%d", (int)rc);
		for (;;) {
			if (mb_tests_strcmp(t->dta.dta_name, "MOVED.TXT") == 0) {
				found_move = 1;
				break;
			}
			rc = Fsnext();
			if (rc != 0)
				break;
		}
		if (rc == 0)
			mb_tests_drain_search();
		if (!found_move)
			mb_panic("FAT test: MOVED.TXT not found in subdir");
	}

	rc = Frename(0, t->rename_dir_src, t->rename_dir_dst);
	if (rc != 0)
		mb_panic("FAT test: dir rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(t->rename_dir_src, 0x17, &t->dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old dir rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = mb_tests_fsfirst(t->newdir_spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: new dir rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = Frename(0, t->rename_dst, t->wrong_drive_rename);
	if (rc != MB_ERR_NSAME)
		mb_panic("FAT test: xdev rename rc=%d expected %d", (int)rc,
			 MB_ERR_NSAME);

	rc = Frename(0, t->missing_rename, t->renamed_spec);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: missing file rename rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = Frename(0, t->missing_dir_rename, t->renamed_spec);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: missing dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = Frename(0, t->renamed_spec, t->missing_dst_dir);
	if (rc != MB_ERR_PTHNF)
		mb_panic("FAT test: missing dst dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Frename(0, t->rename_exist_src, t->rename_exist_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename exists rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	mb_tests_set_readonly("HELLO   TXT");
	rc = Frename(0, t->rename_readonly_src, t->rename_readonly_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename readonly rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = Frename(0, t->rename_readonly_target_src, t->rename_readonly_target_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename readonly target rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = Fattrib(t->ddelete_file, 1, 0);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Fattrib accdn rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	Fsetdta(&t->dta2);
	rc = Fsfirst(t->wildcard, 0x07);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst wildcard rc=%d", (int)rc);
	if (mb_tests_strcmp(t->dta2.dta_name, "HELLO.TXT") != 0)
		mb_panic("FAT test: wildcard name %s", t->dta2.dta_name);
	mb_tests_drain_search();
	Fsetdta(&t->dta);

	rc = mb_tests_fsfirst(t->spec, 0x07, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst attr filter rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(t->dta.dta_name, "SUBDIR") == 0)
			mb_panic("FAT test: SUBDIR should not match");
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc != MB_ERR_NMFIL)
		mb_panic("FAT test: Fsnext end rc=%d expected %d", (int)rc,
			 MB_ERR_NMFIL);

	rc = mb_tests_fsfirst(t->spec, 0x17, &t->dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst dir attr rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(t->dta.dta_name, "NEWDIR") == 0)
			break;
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc != 0)
		mb_panic("FAT test: NEWDIR not found rc=%d", (int)rc);
	mb_tests_drain_search();

	fh = Fopen(t->noslash, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen no slash rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(t->mixed, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen mixed slashes rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(t->ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen seek rc=%d", (int)fh);
	rc = Fseek(-1, (uint16_t)fh, 0);
	if (rc != MB_ERR_RANGE)
		mb_panic("FAT test: Fseek before rc=%d expected %d", (int)rc,
			 MB_ERR_RANGE);
	rc = Fseek(0, (uint16_t)fh, 2);
	if (rc < 0)
		mb_panic("FAT test: Fseek end rc=%d", (int)rc);
	rc = Fread((uint16_t)fh, sizeof(t->buf), t->buf);
	if (rc != 0)
		mb_panic("FAT test: Fread eof rc=%d expected 0", (int)rc);
	rc = Fseek(1, (uint16_t)fh, 2);
	if (rc != MB_ERR_RANGE)
		mb_panic("FAT test: Fseek past end rc=%d expected %d", (int)rc,
			 MB_ERR_RANGE);
	Fclose((uint16_t)fh);

	{
		uint8_t sector[512];
		if (mb_rom_dispatch.rwabs(0, sector, 1, 0,
					  mb_tests_drive_dev) != 0)
			mb_panic("FAT test: rwabs mbr read");
		if (sector[510] != 0x55 || sector[511] != 0xaa)
			mb_panic("FAT test: mbr signature");
		if (mb_rom_dispatch.rwabs(0, sector, 2, 0xffffu,
					  mb_tests_drive_dev) == 0)
			mb_panic("FAT test: rwabs range");
	}
}
