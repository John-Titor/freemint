#include "mb_fat_tests_internal.h"

#include <string.h>

void mb_fat_tests_phase_core_helpers(struct mb_fat_test_ctx *t)
{
	static const uint8_t hello83[11] = {
		'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ', 'T', 'X', 'T'
	};
	uint8_t pat[11];
	uint8_t expect[11];
	uint8_t raw[4];
	char out[14];
	struct mb_fat_dirent ent;
	struct mb_fat_open saved_open;
	struct mb_fat_open op;
	struct mb_fat_volume fake_vol;
	struct mb_fat_volume *saved_vol;
	uint32_t cluster;
	uint32_t cluster_off;

	(void)t;

	raw[0] = 0x34;
	raw[1] = 0x12;
	raw[2] = 0x78;
	raw[3] = 0x56;
	if (mb_fat_le16(raw) != 0x1234u)
		mb_panic("FAT core: le16");
	if (mb_fat_le32(raw) != 0x56781234u)
		mb_panic("FAT core: le32");

	mb_fat_pattern_83("hello.txt", pat);
	memcpy(expect, hello83, sizeof(expect));
	if (mb_tests_memcmp(pat, expect, sizeof(expect)) != 0)
		mb_panic("FAT core: pattern hello");

	mb_fat_pattern_83("*.t?t", pat);
	memset(expect, '?', 8);
	expect[8] = 'T';
	expect[9] = '?';
	expect[10] = 'T';
	if (mb_tests_memcmp(pat, expect, sizeof(expect)) != 0)
		mb_panic("FAT core: pattern wildcard");
	if (!mb_fat_match_83(hello83, pat))
		mb_panic("FAT core: match wildcard");

	if (!mb_fat_has_wildcards("H*.*"))
		mb_panic("FAT core: wildcards expected");
	if (mb_fat_has_wildcards("HELLO.TXT"))
		mb_panic("FAT core: wildcards unexpected");

	memset(&ent, 0, sizeof(ent));
	memcpy(ent.name, hello83, sizeof(hello83));
	mb_fat_name_from_dirent(&ent, out);
	if (mb_tests_strcmp(out, "HELLO.TXT") != 0)
		mb_panic("FAT core: name decode");

	saved_open = mb_fat_open[0];
	memset(&mb_fat_open[0], 0, sizeof(mb_fat_open[0]));
	mb_fat_open[0].in_use = 1;
	if (mb_fat_get_open(3) != &mb_fat_open[0])
		mb_panic("FAT core: get_open valid");
	if (mb_fat_get_open(2) != 0 || mb_fat_get_open(3 + MB_FAT_MAX_OPEN) != 0)
		mb_panic("FAT core: get_open invalid");
	mb_fat_open[0] = saved_open;

	saved_vol = mb_fat_vol;
	memset(&fake_vol, 0, sizeof(fake_vol));
	fake_vol.recsiz = 512;
	fake_vol.spc = 1;
	mb_fat_vol = &fake_vol;

	memset(&op, 0, sizeof(op));
	op.start_cluster = 2;
	if (mb_fat_cluster_for_offset(&op, 100, &cluster, &cluster_off) != 0)
		mb_panic("FAT core: cluster offset");
	if (cluster != 2 || cluster_off != 100)
		mb_panic("FAT core: cluster values");

	op.start_cluster = 1;
	if (mb_fat_cluster_for_offset(&op, 0, &cluster, &cluster_off) == 0)
		mb_panic("FAT core: invalid start cluster");

	op.start_cluster = 2;
	fake_vol.recsiz = 0;
	if (mb_fat_cluster_for_offset(&op, 0, &cluster, &cluster_off) == 0)
		mb_panic("FAT core: zero cluster size");

	mb_fat_vol = saved_vol;

	if (mb_fat_mount(26) != MB_ERR_DRIVE)
		mb_panic("FAT core: mount drive range");
	if (mb_fat_rwabs(0, raw, 1, 0x10000u, 2) == 0)
		mb_panic("FAT core: rwabs recno range");
}
