#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mb_fat_internal.h"
#include "mintboot/mb_tests.h"

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

struct mb_test_bpb {
	uint16_t recsiz;
	int16_t clsiz;
	uint16_t clsizb;
	int16_t rdlen;
	int16_t fsiz;
	int16_t fatrec;
	int16_t datrec;
	uint16_t numcl;
	int16_t bflags;
};

struct mb_test_dir_snap {
	uint32_t start_cluster;
	uint32_t sectors;
	uint32_t buf_offset;
};

struct mb_test_snap {
	uint16_t dev;
	uint16_t recsiz;
	uint16_t spc;
	uint16_t num_fats;
	uint16_t fat_sectors;
	uint32_t fat_start;
	uint32_t root_start;
	uint32_t root_sectors;
	uint16_t dir_count;
	uint32_t dir_used;
	struct mb_test_dir_snap dirs[8];
	uint8_t fat_buf[512 * 512];
	uint8_t root_buf[512 * 512];
	uint8_t dir_buf[256 * 512];
};

static struct mb_test_snap mb_test_snap;

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
	Fsetdta(dta);
	return Fsfirst(spec, attr);
}

static void mb_tests_drain_search(void)
{
	while (Fsnext() == 0) {
	}
}

static uint16_t mb_tests_drive_dev;
static char mb_tests_drive_letter;

static void mb_tests_init_drive(void)
{
	uint16_t dev;

	dev = mb_portable_boot_drive();
	if (dev >= 26)
		mb_panic("FAT test: boot drive invalid");

	mb_tests_drive_dev = dev;
	mb_tests_drive_letter = (char)('A' + dev);
}

static void mb_tests_make_path(char *out, uint32_t outsz, char drive,
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

static void mb_tests_make_noslash_path(char *out, uint32_t outsz,
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

static const struct mb_test_bpb *mb_tests_getbpb(void)
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

static void mb_tests_snapshot_fs(uint16_t dev)
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

static void mb_tests_restore_fs(void)
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

static void mb_tests_set_readonly(const char name83[11])
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

void mb_fat_run_tests(void)
{
	char spec[32];
	struct mb_test_dta dta;
	uint8_t buf[128];
	long rc;
	long fh;
	long n;
	int found = 0;
	const char expect[] = "mintboot FAT16 test file\n";
	const char expect_inner[] = "mintboot nested file\n";
	char missing[32];
	char missing_dir[40];
	char missing_dir_spec[40];
	char wrong_drive[40];
	const char *bad_drive = "1:\\HELLO.TXT";
	char inner_path[40];
	char inner_missing[40];
	char inner_spec[40];
	char rename_src[40];
	char rename_dst[40];
	char move_src[40];
	char move_dst[40];
	char rename_dir_src[40];
	char rename_dir_dst[40];
	char missing_rename[40];
	char missing_dir_rename[40];
	char missing_dst_dir[40];
	char wrong_drive_rename[40];
	char renamed_spec[40];
	char moved_spec[40];
	char newdir_spec[40];
	char dcreate_missing_parent[40];
	char dcreate_root[40];
	char dcreate_exist[40];
	char ddelete_dir[40];
	char ddelete_missing[40];
	char ddelete_file[40];
	char ddelete_missing_dir[40];
	char ddelete_nonempty[40];
	char rename_exist_src[40];
	char rename_exist_dst[40];
	char rename_readonly_src[40];
	char rename_readonly_dst[40];
	char rename_readonly_target_src[40];
	char rename_readonly_target_dst[40];
	int found_inner = 0;
	uint16_t dfree[4];
	uint32_t free_bytes;
	uint32_t total_bytes;
	long rc2;
	int i;
	long handles[MB_FAT_MAX_OPEN];
	struct mb_test_dta dta2;
	const struct mb_test_bpb *bpb;
	uint16_t timebuf[2];
	uint16_t timebuf2[2];
	struct mb_fat_check_report report;
	char other_drive;
	char wildcard[32];
	char noslash[32];
	char mixed[40];

	mb_tests_init_drive();
	mb_tests_snapshot_fs(mb_tests_drive_dev);
	mb_tests_make_path(spec, sizeof(spec), mb_tests_drive_letter, "*.*");
	mb_tests_make_path(missing, sizeof(missing), mb_tests_drive_letter,
			   "NOFILE.TXT");
	mb_tests_make_path(missing_dir, sizeof(missing_dir), mb_tests_drive_letter,
			   "NOSUCH\\HELLO.TXT");
	mb_tests_make_path(missing_dir_spec, sizeof(missing_dir_spec),
			   mb_tests_drive_letter, "NOSUCH\\*.*");
	mb_tests_make_path(inner_path, sizeof(inner_path), mb_tests_drive_letter,
			   "SUBDIR\\INNER.TXT");
	mb_tests_make_path(inner_missing, sizeof(inner_missing),
			   mb_tests_drive_letter, "SUBDIR\\NOFILE.TXT");
	mb_tests_make_path(inner_spec, sizeof(inner_spec), mb_tests_drive_letter,
			   "SUBDIR\\*.*");
	mb_tests_make_path(rename_src, sizeof(rename_src), mb_tests_drive_letter,
			   "RENAME.TXT");
	mb_tests_make_path(rename_dst, sizeof(rename_dst), mb_tests_drive_letter,
			   "RENAMED.TXT");
	mb_tests_make_path(move_src, sizeof(move_src), mb_tests_drive_letter,
			   "MOVE.TXT");
	mb_tests_make_path(move_dst, sizeof(move_dst), mb_tests_drive_letter,
			   "SUBDIR\\MOVED.TXT");
	mb_tests_make_path(rename_dir_src, sizeof(rename_dir_src),
			   mb_tests_drive_letter, "SUBDIR");
	mb_tests_make_path(rename_dir_dst, sizeof(rename_dir_dst),
			   mb_tests_drive_letter, "NEWDIR");
	mb_tests_make_path(missing_rename, sizeof(missing_rename),
			   mb_tests_drive_letter, "NOFILE.TXT");
	mb_tests_make_path(missing_dir_rename, sizeof(missing_dir_rename),
			   mb_tests_drive_letter, "NODIR");
	mb_tests_make_path(missing_dst_dir, sizeof(missing_dst_dir),
			   mb_tests_drive_letter, "NOSUCH\\RENAMED.TXT");
	mb_tests_make_path(renamed_spec, sizeof(renamed_spec),
			   mb_tests_drive_letter, "RENAMED.TXT");
	mb_tests_make_path(moved_spec, sizeof(moved_spec), mb_tests_drive_letter,
			   "SUBDIR\\MOVED.TXT");
	mb_tests_make_path(newdir_spec, sizeof(newdir_spec), mb_tests_drive_letter,
			   "NEWDIR");
	mb_tests_make_path(dcreate_missing_parent, sizeof(dcreate_missing_parent),
			   mb_tests_drive_letter, "NOSUCH\\NEW.DIR");
	mb_tests_make_path(dcreate_root, sizeof(dcreate_root),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(dcreate_exist, sizeof(dcreate_exist),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(ddelete_dir, sizeof(ddelete_dir), mb_tests_drive_letter,
			   "NEWDIR2");
	mb_tests_make_path(ddelete_missing, sizeof(ddelete_missing),
			   mb_tests_drive_letter, "NEWDIR2");
	mb_tests_make_path(ddelete_file, sizeof(ddelete_file),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(ddelete_missing_dir, sizeof(ddelete_missing_dir),
			   mb_tests_drive_letter, "NOPE");
	mb_tests_make_path(ddelete_nonempty, sizeof(ddelete_nonempty),
			   mb_tests_drive_letter, "NEWDIR");
	mb_tests_make_path(rename_exist_src, sizeof(rename_exist_src),
			   mb_tests_drive_letter, "RENAMED.TXT");
	mb_tests_make_path(rename_exist_dst, sizeof(rename_exist_dst),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(rename_readonly_src, sizeof(rename_readonly_src),
			   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(rename_readonly_dst, sizeof(rename_readonly_dst),
			   mb_tests_drive_letter, "HELLO2.TXT");
	mb_tests_make_path(rename_readonly_target_src,
			   sizeof(rename_readonly_target_src),
			   mb_tests_drive_letter, "NEWDIR\\INNER.TXT");
	mb_tests_make_path(rename_readonly_target_dst,
			   sizeof(rename_readonly_target_dst),
			   mb_tests_drive_letter, "HELLO.TXT");

	if (mb_tests_drive_letter == 'Z')
		other_drive = 'Y';
	else
		other_drive = (char)(mb_tests_drive_letter + 1);
	mb_tests_make_path(wrong_drive, sizeof(wrong_drive), other_drive,
			   "HELLO.TXT");
	mb_tests_make_path(wrong_drive_rename, sizeof(wrong_drive_rename),
			   other_drive, "RENAMED.TXT");
	mb_tests_make_path(wildcard, sizeof(wildcard), mb_tests_drive_letter,
			   "H*.TXT");
	mb_tests_make_noslash_path(noslash, sizeof(noslash),
				   mb_tests_drive_letter, "HELLO.TXT");
	mb_tests_make_path(mixed, sizeof(mixed), mb_tests_drive_letter,
			   "NEWDIR\\INNER.TXT");
	mixed[2] = '/';

	mb_log_puts("mintboot: FAT test start\r\n");
	bpb = mb_tests_getbpb();
	if (bpb->clsiz <= 0 || bpb->rdlen <= 0 || bpb->datrec <= 0 ||
	    bpb->numcl == 0)
		mb_panic("FAT test: BPB fields invalid");

	rc = mb_tests_fsfirst(spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "HELLO.TXT") == 0) {
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

	fh = Fopen(ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT failed");

	n = Fread((uint16_t)fh, sizeof(buf), buf);
	Fclose((uint16_t)fh);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++) {
		handles[i] = Fopen(ddelete_file, 0);
		if (handles[i] < 0)
			mb_panic("FAT test: Fopen bulk rc=%d at %d",
				 (int)handles[i], i);
	}
	fh = Fopen(ddelete_file, 0);
	if (fh != MB_ERR_NHNDL)
		mb_panic("FAT test: Fopen ENHNDL rc=%d expected %d", (int)fh,
			 MB_ERR_NHNDL);
	for (i = 0; i < MB_FAT_MAX_OPEN; i++) {
		Fclose((uint16_t)handles[i]);
	}
	if (n != (long)(sizeof(expect) - 1))
		mb_panic("FAT test: Fread size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect,
			    (uint32_t)(sizeof(expect) - 1)) != 0)
		mb_panic("FAT test: HELLO.TXT contents mismatch");

	rc = Fclose(1);
	if (rc != MB_ERR_IHNDL)
		mb_panic("FAT test: Fclose badf rc=%d expected %d", (int)rc,
			 MB_ERR_IHNDL);

	rc = Fread(1, sizeof(buf), buf);
	if (rc != MB_ERR_IHNDL)
		mb_panic("FAT test: Fread badf rc=%d expected %d", (int)rc,
			 MB_ERR_IHNDL);

	fh = Fopen(ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT for lock failed");
	Fclose((uint16_t)fh);

	fh = Fopen(missing, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen missing rc=%d expected %d", (int)fh,
			 MB_ERR_FILNF);

	fh = Fopen(missing_dir, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen missing dir rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	Fsetdta(&dta);
	rc = Fsfirst(missing_dir_spec, 0x17);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Fsfirst missing dir rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(spec, 0x17, &dta);
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

	rc = mb_tests_fsfirst(inner_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst inner rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "INNER.TXT") == 0) {
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

	fh = Fopen(inner_path, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen INNER.TXT failed");
	n = Fread((uint16_t)fh, sizeof(buf), buf);
	Fclose((uint16_t)fh);
	if (n != (long)(sizeof(expect_inner) - 1))
		mb_panic("FAT test: INNER.TXT size %u expected %u",
			 (unsigned int)n,
			 (unsigned int)(sizeof(expect_inner) - 1));
	if (mb_tests_memcmp(buf, (const uint8_t *)expect_inner,
			    (uint32_t)(sizeof(expect_inner) - 1)) != 0)
		mb_panic("FAT test: INNER.TXT contents mismatch");

	fh = Fopen(inner_missing, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen inner missing rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(wrong_drive, 0x17, &dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Fsfirst wrong drive rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	fh = Fopen(wrong_drive, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen wrong drive rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	rc = mb_tests_fsfirst(bad_drive, 0x17, &dta);
	if (rc != MB_ERR_DRIVE)
		mb_panic("FAT test: Fsfirst bad drive rc=%d expected %d",
			 (int)rc, MB_ERR_DRIVE);

	fh = Fopen(bad_drive, 0);
	if (fh != MB_ERR_FILNF)
		mb_panic("FAT test: Fopen bad drive rc=%d expected %d",
			 (int)fh, MB_ERR_FILNF);

	if (Dfree(dfree, mb_tests_drive_dev) != 0)
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
	fh = Fopen(rename_src, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen rename src rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen HELLO.TXT for fdatime rc=%d", (int)fh);
	if (Fdatime((uint32_t)(uintptr_t)timebuf, (uint16_t)fh, 0) != 0)
		mb_panic("FAT test: Fdatime failed");
	if (Fdatime((uint32_t)(uintptr_t)timebuf2, (uint16_t)fh, 0) != 0)
		mb_panic("FAT test: Fdatime repeat failed");
	Fclose((uint16_t)fh);
	if ((timebuf[0] == 0 && timebuf[1] == 0) ||
	    timebuf[0] != timebuf2[0] || timebuf[1] != timebuf2[1])
		mb_panic("FAT test: Fdatime mismatch");

	rc = Frename(0, rename_src, rename_dst);
	if (rc != 0)
		mb_panic("FAT test: Frename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(rename_src, 0x17, &dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old rename rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = mb_tests_fsfirst(renamed_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: new rename rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = Frename(0, move_src, move_dst);
	if (rc != 0)
		mb_panic("FAT test: move rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(move_src, 0x17, &dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old move rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
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
			rc = Fsnext();
			if (rc != 0)
				break;
		}
		if (rc == 0)
			mb_tests_drain_search();
		if (!found_move)
			mb_panic("FAT test: MOVED.TXT not found in subdir");
	}

	rc = Frename(0, rename_dir_src, rename_dir_dst);
	if (rc != 0)
		mb_panic("FAT test: dir rename rc=%d expected 0", (int)rc);
	rc = mb_tests_fsfirst(rename_dir_src, 0x17, &dta);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: old dir rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = mb_tests_fsfirst(newdir_spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: new dir rc=%d expected 0", (int)rc);
	mb_tests_drain_search();

	rc = Frename(0, rename_dst, wrong_drive_rename);
	if (rc != MB_ERR_NSAME)
		mb_panic("FAT test: xdev rename rc=%d expected %d", (int)rc,
			 MB_ERR_NSAME);

	rc = Frename(0, missing_rename, renamed_spec);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: missing file rename rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = Frename(0, missing_dir_rename, renamed_spec);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: missing dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);

	rc = Frename(0, renamed_spec, missing_dst_dir);
	if (rc != MB_ERR_PTHNF)
		mb_panic("FAT test: missing dst dir rename rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Frename(0, rename_exist_src, rename_exist_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename exists rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	mb_tests_set_readonly("HELLO   TXT");
	rc = Frename(0, rename_readonly_src, rename_readonly_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename readonly rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = Frename(0, rename_readonly_target_src, rename_readonly_target_dst);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: rename readonly target rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = Fattrib(ddelete_file, 1, 0);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Fattrib accdn rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	Fsetdta(&dta2);
	rc = Fsfirst(wildcard, 0x07);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst wildcard rc=%d", (int)rc);
	if (mb_tests_strcmp(dta2.dta_name, "HELLO.TXT") != 0)
		mb_panic("FAT test: wildcard name %s", dta2.dta_name);
	mb_tests_drain_search();
	Fsetdta(&dta);

	rc = mb_tests_fsfirst(spec, 0x07, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst attr filter rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "SUBDIR") == 0)
			mb_panic("FAT test: SUBDIR should not match");
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc != MB_ERR_NMFIL)
		mb_panic("FAT test: Fsnext end rc=%d expected %d", (int)rc,
			 MB_ERR_NMFIL);

	rc = mb_tests_fsfirst(spec, 0x17, &dta);
	if (rc != 0)
		mb_panic("FAT test: Fsfirst dir attr rc=%d", (int)rc);
	for (;;) {
		if (mb_tests_strcmp(dta.dta_name, "NEWDIR") == 0)
			break;
		rc = Fsnext();
		if (rc != 0)
			break;
	}
	if (rc != 0)
		mb_panic("FAT test: NEWDIR not found rc=%d", (int)rc);
	mb_tests_drain_search();

	fh = Fopen(noslash, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen no slash rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(mixed, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen mixed slashes rc=%d", (int)fh);
	Fclose((uint16_t)fh);

	fh = Fopen(ddelete_file, 0);
	if (fh < 0)
		mb_panic("FAT test: Fopen seek rc=%d", (int)fh);
	rc = Fseek(-1, (uint16_t)fh, 0);
	if (rc != MB_ERR_RANGE)
		mb_panic("FAT test: Fseek before rc=%d expected %d", (int)rc,
			 MB_ERR_RANGE);
	rc = Fseek(0, (uint16_t)fh, 2);
	if (rc < 0)
		mb_panic("FAT test: Fseek end rc=%d", (int)rc);
	rc = Fread((uint16_t)fh, sizeof(buf), buf);
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

	rc = Dcreate(dcreate_missing_parent);
	if (rc != MB_ERR_PTHNF)
		mb_panic("FAT test: Dcreate missing parent rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Dcreate(dcreate_root);
	if (rc != 0)
		mb_panic("FAT test: Dcreate rc=%d expected 0", (int)rc);

	rc = Dcreate(dcreate_exist);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Dcreate exists rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	rc = Ddelete(ddelete_dir);
	if (rc != 0)
		mb_panic("FAT test: Ddelete dir rc=%d expected 0", (int)rc);
	rc = Ddelete(ddelete_missing);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Ddelete missing rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = Ddelete(ddelete_file);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Ddelete file rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);
	rc = Ddelete(ddelete_missing_dir);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Ddelete missing dir rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);
	rc = Ddelete(ddelete_nonempty);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Ddelete nonempty rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = mb_fat_check(mb_tests_drive_dev, &report);
	if (rc != 0)
		mb_panic("FAT test: integrity rc=%d bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 (int)rc, report.bad_bpb, report.bad_dirent,
			 report.bad_chain, report.bad_fat, report.lost_clusters,
			 report.cross_links);
	if (report.bad_bpb || report.bad_dirent || report.bad_chain ||
	    report.bad_fat || report.lost_clusters || report.cross_links)
		mb_panic("FAT test: integrity bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 report.bad_bpb, report.bad_dirent, report.bad_chain,
			 report.bad_fat, report.lost_clusters, report.cross_links);

	mb_log_puts("mintboot: FAT test done\r\n");
	mb_tests_restore_fs();
}
