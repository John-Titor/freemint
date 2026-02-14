#include "mb_fat_tests_internal.h"

struct mb_test_snap mb_test_snap;
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
	if (dev >= 26)
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

	bpb = (const struct mb_test_bpb *)(uintptr_t)Getbpb(mb_tests_drive_dev);
	if (!bpb || bpb->recsiz != 512)
		mb_panic("FAT test: BPB invalid");
	return bpb;
}

static void mb_tests_snapshot_dir(uint32_t start_cluster)
{
	struct mb_test_snap *snap = &mb_test_snap;
	uint32_t cluster = start_cluster;
	uint32_t sectors = 0;
	uint32_t offset = snap->dir_used;
	uint32_t steps = 0;

	if (snap->dir_count >= (uint16_t)(sizeof(snap->dirs) /
					 sizeof(snap->dirs[0])))
		mb_panic("FAT test: dir snapshot overflow");

	while (cluster >= 2u && cluster < 0xfff8u) {
		uint32_t sector;
		uint16_t i;

		if (steps++ > 0xffffu)
			mb_panic("FAT test: dir chain loop");

		if (offset + sectors + snap->spc >
		    (uint32_t)(sizeof(snap->dir_buf) / 512u))
			mb_panic("FAT test: dir snapshot too large");

		sector = mb_fat_cluster_sector(cluster);
		for (i = 0; i < snap->spc; i++) {
			if (mb_fat_rwabs(0,
					 snap->dir_buf +
					 (offset + sectors + i) * 512u,
					 1, sector + i, snap->dev) != 0)
				mb_panic("FAT test: dir snapshot read");
		}
		sectors += snap->spc;
		cluster = mb_fat_read_fat(cluster);
	}

	snap->dirs[snap->dir_count].start_cluster = start_cluster;
	snap->dirs[snap->dir_count].sectors = sectors;
	snap->dirs[snap->dir_count].buf_offset = offset;
	snap->dir_count++;
	snap->dir_used += sectors;
}

void mb_tests_snapshot_fs(uint16_t dev)
{
	struct mb_test_snap *snap = &mb_test_snap;
	struct mb_fat_dirent ent;
	uint32_t total_fat_sectors;
	uint32_t i;

	if (mb_fat_mount(dev) != 0 || !mb_fat_vol)
		mb_panic("FAT test: snapshot mount failed");

	snap->dev = dev;
	snap->recsiz = mb_fat_vol->recsiz;
	snap->spc = mb_fat_vol->spc;
	snap->num_fats = mb_fat_vol->num_fats;
	snap->fat_sectors = mb_fat_vol->fat_sectors;
	snap->fat_start = mb_fat_vol->fat_start;
	snap->root_start = mb_fat_vol->root_start;
	snap->root_sectors = mb_fat_vol->root_sectors;
	snap->dir_count = 0;
	snap->dir_used = 0;

	if (snap->recsiz != 512)
		mb_panic("FAT test: snapshot recsiz=%u", snap->recsiz);

	total_fat_sectors = (uint32_t)snap->num_fats * snap->fat_sectors;
	if (total_fat_sectors > (uint32_t)(sizeof(snap->fat_buf) / 512u))
		mb_panic("FAT test: snapshot FAT too large");
	if (snap->root_sectors > (uint32_t)(sizeof(snap->root_buf) / 512u))
		mb_panic("FAT test: snapshot root too large");

	for (i = 0; i < total_fat_sectors; i++) {
		if (mb_fat_rwabs(0, snap->fat_buf + i * 512u, 1,
				 snap->fat_start + i, dev) != 0)
			mb_panic("FAT test: snapshot FAT read");
	}

	for (i = 0; i < snap->root_sectors; i++) {
		if (mb_fat_rwabs(0, snap->root_buf + i * 512u, 1,
				 snap->root_start + i, dev) != 0)
			mb_panic("FAT test: snapshot root read");
	}

	for (i = 0; i < (snap->root_sectors * 512u) / 32u; i++) {
		char name[14];
		uint32_t cluster;

		if (mb_fat_read_dirent(0, i, &ent) != 0)
			mb_panic("FAT test: snapshot root dirent");
		if (ent.name[0] == 0x00)
			break;
		if (ent.name[0] == 0xe5)
			continue;
		if ((ent.attr & MB_FAT_ATTR_DIR) == 0)
			continue;
		if (ent.attr & MB_FAT_ATTR_LABEL)
			continue;
		mb_fat_name_from_dirent(&ent, name);
		if ((name[0] == '.' && name[1] == '\0') ||
		    (name[0] == '.' && name[1] == '.' && name[2] == '\0'))
			continue;
		cluster = ((uint32_t)ent.start_hi << 16) | ent.start_lo;
		if (cluster < 2u)
			continue;
		mb_tests_snapshot_dir(cluster);
	}
}

void mb_tests_restore_fs(void)
{
	struct mb_test_snap *snap = &mb_test_snap;
	uint32_t total_fat_sectors;
	uint32_t i;
	uint16_t d;

	if (mb_fat_mount(snap->dev) != 0 || !mb_fat_vol)
		mb_panic("FAT test: restore mount failed");

	d = snap->dev;
	total_fat_sectors = (uint32_t)snap->num_fats * snap->fat_sectors;
	for (i = 0; i < total_fat_sectors; i++) {
		if (mb_fat_rwabs(1, snap->fat_buf + i * 512u, 1,
				 snap->fat_start + i, d) != 0)
			mb_panic("FAT test: restore FAT write");
	}

	for (i = 0; i < snap->root_sectors; i++) {
		if (mb_fat_rwabs(1, snap->root_buf + i * 512u, 1,
				 snap->root_start + i, d) != 0)
			mb_panic("FAT test: restore root write");
	}

	for (i = 0; i < snap->dir_count; i++) {
		uint32_t cluster = snap->dirs[i].start_cluster;
		uint32_t sector_off = snap->dirs[i].buf_offset;
		uint32_t sectors_left = snap->dirs[i].sectors;
		uint32_t steps = 0;

		while (cluster >= 2u && cluster < 0xfff8u && sectors_left) {
			uint32_t sector = mb_fat_cluster_sector(cluster);
			uint16_t j;

			if (steps++ > 0xffffu)
				mb_panic("FAT test: restore chain loop");

			for (j = 0; j < snap->spc && sectors_left; j++) {
				if (mb_fat_rwabs(1,
						 snap->dir_buf +
						 (sector_off * 512u),
						 1, sector + j, d) != 0)
					mb_panic("FAT test: restore dir write");
				sector_off++;
				sectors_left--;
			}
			cluster = mb_fat_read_fat(cluster);
		}
	}
}

void mb_tests_set_readonly(const char name83[11])
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
		sector[offset + 11] |= 0x01;
		if (mb_rom_dispatch.rwabs(1, sector, 1, (uint16_t)sector_idx,
					  mb_tests_drive_dev) != 0)
			mb_panic("FAT test: rwabs write root");
		return;
	}

	mb_panic("FAT test: readonly target not found");
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

	if (mb_tests_drive_letter == 'Z')
		other_drive = 'Y';
	else
		other_drive = (char)(mb_tests_drive_letter + 1);
	mb_tests_make_path(t->wrong_drive, sizeof(t->wrong_drive), other_drive,
			   "HELLO.TXT");
	mb_tests_make_noslash_path(t->noslash, sizeof(t->noslash),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(t->mixed, sizeof(t->mixed), mb_tests_drive_letter,
			   "SUBDIR\\INNER.TXT");
	t->mixed[2] = '/';
}
