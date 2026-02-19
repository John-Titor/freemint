#include "mb_fat_tests_internal.h"
#include "mintboot/mb_errors.h"

uint16_t mb_tests_drive_dev;
char mb_tests_drive_letter;

int mb_tests_strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

int mb_tests_memcmp(const uint8_t *a, const uint8_t *b, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		if (a[i] != b[i])
			return (int)a[i] - (int)b[i];
	}
	return 0;
}

void mb_tests_init_drive(void)
{
	uint16_t dev;

	dev = mb_common_boot_drive();
	if (dev >= MB_MAX_DRIVES)
		mb_panic("FAT test: boot drive invalid");

	mb_tests_drive_dev = dev;
	mb_tests_drive_letter = (char)('A' + dev);
}

void mb_tests_make_path(char *out, uint32_t outsz, char drive,
			const char *suffix)
{
	uint32_t i = 0;
	uint32_t j = 0;

	if (outsz < 4)
		mb_panic("FAT test: path buffer too small");

	out[i++] = drive;
	out[i++] = ':';
	out[i++] = '\\';

	if (suffix[0] == '\\' || suffix[0] == '/')
		j = 1;
	while (suffix[j] && i + 1 < outsz) {
		out[i++] = suffix[j++];
	}
	out[i] = '\0';

	if (suffix[j])
		mb_panic("FAT test: path overflow");
}

void mb_tests_make_noslash_path(char *out, uint32_t outsz,
				char drive, const char *suffix)
{
	uint32_t i = 0;
	uint32_t j = 0;

	if (outsz < 3)
		mb_panic("FAT test: path buffer too small");

	out[i++] = drive;
	out[i++] = ':';

	if (suffix[0] == '\\' || suffix[0] == '/')
		j = 1;
	while (suffix[j] && i + 1 < outsz) {
		out[i++] = suffix[j++];
	}
	out[i] = '\0';

	if (suffix[j])
		mb_panic("FAT test: path overflow");
}

const struct mb_test_bpb *mb_tests_getbpb(void)
{
	const struct mb_test_bpb *bpb;
	long bpb_ptr;

	bpb_ptr = (long)(intptr_t)Getbpb(mb_tests_drive_dev);
	bpb = (const struct mb_test_bpb *)(uintptr_t)bpb_ptr;
	if (!bpb || bpb->recsiz != 512) {
		mb_log_printf("FAT test: Getbpb dev=%u ptr=0x%08lx recsiz=%u\n",
			      (unsigned)mb_tests_drive_dev,
			      (unsigned long)bpb_ptr,
			      bpb ? (unsigned)bpb->recsiz : 0u);
		mb_panic("FAT test: BPB invalid");
	}
	return bpb;
}

static int mb_tests_update_attr(const char name83[11], uint8_t set_mask,
			       uint8_t clear_mask)
{
	const struct mb_test_bpb *bpb;
	uint8_t sector[512];
	uint32_t root_start;
	uint32_t entries_per_sector;
	uint32_t total_entries;
	uint32_t i;

	bpb = mb_tests_getbpb();

	root_start = (uint32_t)bpb->datrec - (uint32_t)bpb->rdlen;
	entries_per_sector = bpb->recsiz / 32u;
	total_entries = (uint32_t)bpb->rdlen * entries_per_sector;

	for (i = 0; i < total_entries; i++) {
		uint32_t sector_idx = root_start + (i / entries_per_sector);
		uint32_t offset = (i % entries_per_sector) * 32u;
		if (mb_rom_dispatch.rwabs(0, sector, 1, (uint16_t)sector_idx,
					  mb_tests_drive_dev) != 0)
			mb_panic("FAT test: rwabs read root");
		if (sector[offset] == 0x00)
			break;
		if (sector[offset] == 0xe5)
			continue;
		if (mb_tests_memcmp(&sector[offset], (const uint8_t *)name83, 11) != 0)
			continue;
		sector[offset + 11] = (uint8_t)((sector[offset + 11] | set_mask) &
						(uint8_t)~clear_mask);
		if (mb_rom_dispatch.rwabs(1, sector, 1, (uint16_t)sector_idx,
					  mb_tests_drive_dev) != 0)
			mb_panic("FAT test: rwabs write root");
		return 0;
	}

	return MB_ERR_FILNF;
}

void mb_tests_set_readonly(const char name83[11])
{
	if (mb_tests_update_attr(name83, 0x01u, 0u) != 0)
		mb_panic("FAT test: readonly target not found");
}

static void mb_tests_clear_readonly_if_exists(const char name83[11])
{
	(void)mb_tests_update_attr(name83, 0u, 0x01u);
}

static void mb_tests_delete_if_exists(const char *path)
{
	long rc = Fdelete(path);

	if (rc == 0 || rc == MB_ERR_FILNF)
		return;
	mb_panic("FAT test: cleanup delete rc=%d path='%s'", (int)rc, path);
}

void mb_fat_tests_init_context(struct mb_fat_test_ctx *t)
{
	char other_drive;

	mb_tests_make_path(t->spec, sizeof(t->spec), mb_tests_drive_letter, "*.*");
	mb_tests_make_path(t->missing, sizeof(t->missing), mb_tests_drive_letter,
			   "NOFILE.TXT");
	mb_tests_make_path(t->missing_dir, sizeof(t->missing_dir), mb_tests_drive_letter,
			   "NOSUCH\\HELLO.TXT");
	mb_tests_make_path(t->inner_path, sizeof(t->inner_path), mb_tests_drive_letter,
			   "SUBDIR\\INNER.TXT");
	mb_tests_make_path(t->inner_missing, sizeof(t->inner_missing),
			   mb_tests_drive_letter, "SUBDIR\\NOFILE.TXT");
	mb_tests_make_path(t->dcreate_missing_parent, sizeof(t->dcreate_missing_parent),
			   mb_tests_drive_letter, "NOSUCH\\NEW.DIR");
	mb_tests_make_path(t->dcreate_root, sizeof(t->dcreate_root),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(t->dcreate_exist, sizeof(t->dcreate_exist),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(t->ddelete_dir, sizeof(t->ddelete_dir), mb_tests_drive_letter,
			   "NEWDIR2");
	mb_tests_make_path(t->ddelete_missing, sizeof(t->ddelete_missing),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(t->ddelete_file, sizeof(t->ddelete_file),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(t->ddelete_missing_dir, sizeof(t->ddelete_missing_dir),
			   mb_tests_drive_letter, "NOPE");
	mb_tests_make_path(t->ddelete_nonempty, sizeof(t->ddelete_nonempty),
			   mb_tests_drive_letter, "NEWDIR");
	mb_tests_make_path(t->fcreate_new, sizeof(t->fcreate_new), mb_tests_drive_letter,
			   "NEWFILE.TXT");
	mb_tests_make_path(t->fcreate_rewrite, sizeof(t->fcreate_rewrite), mb_tests_drive_letter,
			   "REWRITE.TXT");
	mb_tests_make_path(t->fcreate_missing_parent, sizeof(t->fcreate_missing_parent),
			   mb_tests_drive_letter, "NOSUCH\\NEWFILE.TXT");
	mb_tests_make_path(t->fcreate_dir_target, sizeof(t->fcreate_dir_target),
			   mb_tests_drive_letter, "SUBDIR");
	mb_tests_make_path(t->fdelete_missing_file, sizeof(t->fdelete_missing_file),
			   mb_tests_drive_letter, "NOFILE2.TXT");
	mb_tests_make_path(t->fdelete_dir_target, sizeof(t->fdelete_dir_target),
			   mb_tests_drive_letter, "SUBDIR");

	if (mb_tests_drive_letter == (char)('A' + (MB_MAX_DRIVES - 1u)))
		other_drive = 'A';
	else
		other_drive = (char)(mb_tests_drive_letter + 1);
	mb_tests_make_path(t->wrong_drive, sizeof(t->wrong_drive), other_drive,
			   "HELLO.TXT");
	mb_tests_make_noslash_path(t->noslash, sizeof(t->noslash),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(t->mixed, sizeof(t->mixed), mb_tests_drive_letter,
			   "SUBDIR\\INNER.TXT");
	t->mixed[2] = '/';

	/* Keep tests repeatable with persistent disk images across runs. */
	mb_tests_clear_readonly_if_exists("NEWFILE TXT");
	mb_tests_clear_readonly_if_exists("REWRITE TXT");
	mb_tests_delete_if_exists(t->fcreate_new);
	mb_tests_delete_if_exists(t->fcreate_rewrite);
}
