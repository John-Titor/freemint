#include "mb_fat_tests_internal.h"

void mb_fat_tests_phase_rename_attr_seek(struct mb_fat_test_ctx *t)
{
	long rc;
	long fh;

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
