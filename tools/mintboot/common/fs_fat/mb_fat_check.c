#include "mb_fat_internal.h"
#include "mintboot/mb_fat.h"

#include "mintboot/mb_lib.h"

#define MB_FAT_MAX_CLUSTERS 65536
#define MB_FAT_MAX_DIR_DEPTH 64

static uint16_t mb_fat_cluster_refs[MB_FAT_MAX_CLUSTERS];
static uint16_t mb_fat_cluster_marks[MB_FAT_MAX_CLUSTERS];
static uint16_t mb_fat_dir_marks[MB_FAT_MAX_CLUSTERS];

static int mb_fat_valid_cluster(uint32_t cluster)
{
	uint32_t max_cluster = (uint32_t)mb_fat_vol->total_clusters + 1u;

	if (cluster < 2 || cluster > max_cluster)
		return 0;
	return 1;
}

static int mb_fat_chain_mark(uint32_t start_cluster, uint32_t size,
			     uint16_t chain_id,
			     struct mb_fat_check_report *report,
			     uint32_t *clusters_seen_out)
{
	uint32_t cluster = start_cluster;
	uint32_t cluster_size;
	uint32_t clusters_seen = 0;

	if (start_cluster < 2)
		return 0;

	cluster_size = (uint32_t)mb_fat_vol->spc * mb_fat_vol->recsiz;
	while (cluster >= 2 && cluster < 0xfff8u) {
		uint16_t next;

		if (!mb_fat_valid_cluster(cluster)) {
			report->bad_chain++;
			return -1;
		}

		if (mb_fat_cluster_marks[cluster] == chain_id) {
			report->bad_chain++;
			return -1;
		}
		mb_fat_cluster_marks[cluster] = chain_id;

		mb_fat_cluster_refs[cluster]++;
		if (mb_fat_cluster_refs[cluster] > 1)
			report->cross_links++;

		clusters_seen++;
		next = mb_fat_read_fat(cluster);
		if (next == 0 || next == 0x0001u || next == 0xfff7u ||
		    (next >= 0xfff0u && next <= 0xfff6u) ||
		    (next >= 2 && next <= 0xffefu &&
		     !mb_fat_valid_cluster(next))) {
			report->bad_fat++;
			return -1;
		}

		if (next >= 0xfff8u)
			break;

		cluster = next;
	}

	if (size && cluster_size) {
		uint32_t max_size = clusters_seen * cluster_size;
		if (size > max_size)
			report->bad_chain++;
	}

	if (clusters_seen_out)
		*clusters_seen_out = clusters_seen;

	return 0;
}

static int mb_fat_check_dir(uint32_t dir_cluster, uint16_t *chain_id,
			    uint32_t *dir_stack, int *sp,
			    struct mb_fat_check_report *report)
{
	uint32_t idx = 0;
	uint8_t raw[32];
	uint8_t dot_count = 0;
	uint8_t dotdot_count = 0;
	int is_root = (dir_cluster == 0);

	for (;;) {
		if (mb_fat_read_dirent_raw(dir_cluster, idx, raw) != 0) {
			report->bad_dirent++;
			return -1;
		}

		if (raw[0] == 0x00)
			break;
		if (raw[0] == 0xe5 || raw[11] == MB_FAT_ATTR_LFN) {
			idx++;
			continue;
		}

		{
			uint8_t attr = raw[11];
			uint16_t start_lo = mb_fat_le16(&raw[26]);
			uint32_t size = mb_fat_le32(&raw[28]);
			int is_dot = (raw[0] == '.' && raw[1] == ' ');
			int is_dotdot = (raw[0] == '.' && raw[1] == '.');
			uint32_t i;

			if (is_dot || is_dotdot) {
				/* skip name validation */
			} else if (attr & MB_FAT_ATTR_LABEL) {
				if (!is_root)
					report->bad_dirent++;
			} else {
				for (i = 0; i < 11; i++) {
					uint8_t c = raw[i];
					if (c == 0x05)
						c = 0xe5;
					if (c == ' ')
						continue;
					if (c >= 'A' && c <= 'Z')
						continue;
					if (c >= '0' && c <= '9')
						continue;
					if (c == '$' || c == '%' || c == '\'' ||
					    c == '-' || c == '_' || c == '@' ||
					    c == '~' || c == '`' || c == '!' ||
					    c == '(' || c == ')' || c == '{' ||
					    c == '}' || c == '^' || c == '#' ||
					    c == '&')
						continue;
					report->bad_dirent++;
					break;
				}
			}

			if (is_dot)
				dot_count++;
			if (is_dotdot)
				dotdot_count++;

			if (start_lo != 0 && !mb_fat_valid_cluster(start_lo))
				report->bad_dirent++;

			if (!(attr & MB_FAT_ATTR_DIR)) {
				if (start_lo < 2 && size > 0)
					report->bad_dirent++;
				if (start_lo >= 2 && size == 0)
					report->bad_dirent++;
			}

			if ((attr & MB_FAT_ATTR_DIR) && !is_dot && !is_dotdot &&
			    start_lo >= 2) {
				if (mb_fat_dir_marks[start_lo] == *chain_id) {
					report->bad_chain++;
				} else if (*sp < MB_FAT_MAX_DIR_DEPTH) {
					mb_fat_dir_marks[start_lo] = *chain_id;
					dir_stack[(*sp)++] = start_lo;
				} else {
					report->bad_chain++;
				}
			}

			if (start_lo >= 2 && !is_dot && !is_dotdot) {
				uint32_t clusters_seen = 0;
				if (mb_fat_chain_mark(start_lo, size,
						      (*chain_id)++, report,
						      &clusters_seen) != 0)
					return -1;
				if (*chain_id == 0) {
					memset(mb_fat_cluster_marks, 0,
					       sizeof(mb_fat_cluster_marks));
					*chain_id = 1;
				}
			}
		}

		idx++;
	}

	if (is_root) {
		if (dot_count || dotdot_count)
			report->bad_dirent++;
	} else {
		if (dot_count != 1 || dotdot_count != 1)
			report->bad_dirent++;
	}

	return 0;
}

long mb_fat_check(uint16_t dev, struct mb_fat_check_report *report)
{
	uint16_t chain_id = 1;
	uint32_t dir_stack[MB_FAT_MAX_DIR_DEPTH];
	int sp = 0;
	uint32_t cluster;

	if (report)
		memset(report, 0, sizeof(*report));

	if (dev > 25)
		return MB_ERR_DRIVE;

	if (mb_fat_mount(dev) != 0)
		return MB_ERR_FILNF;

	if (mb_fat_vol->recsiz != 512 || mb_fat_vol->spc == 0 ||
	    mb_fat_vol->num_fats == 0 || mb_fat_vol->fat_sectors == 0 ||
	    mb_fat_vol->root_sectors == 0 || mb_fat_vol->data_start == 0 ||
	    mb_fat_vol->total_clusters == 0) {
		if (report)
			report->bad_bpb++;
		return MB_ERR_ACCDN;
	}

	if (mb_fat_vol->total_clusters + 2u > MB_FAT_MAX_CLUSTERS)
		return MB_ERR_ACCDN;

	memset(mb_fat_cluster_refs, 0, sizeof(mb_fat_cluster_refs));
	memset(mb_fat_cluster_marks, 0, sizeof(mb_fat_cluster_marks));
	memset(mb_fat_dir_marks, 0, sizeof(mb_fat_dir_marks));

	dir_stack[sp++] = 0;
	while (sp > 0) {
		uint32_t dir_cluster = dir_stack[--sp];
		if (mb_fat_check_dir(dir_cluster, &chain_id, dir_stack, &sp,
				     report) != 0)
			return MB_ERR_ACCDN;
	}

	for (cluster = 2; cluster < (uint32_t)mb_fat_vol->total_clusters + 2u;
	     cluster++) {
		uint16_t fat = mb_fat_read_fat(cluster);
		if (fat != 0 && mb_fat_cluster_refs[cluster] == 0)
			report->lost_clusters++;
		if (fat == 0xfff7u)
			report->bad_fat++;
		if (fat == 0x0001u ||
		    (fat >= 0xfff0u && fat <= 0xfff6u))
			report->bad_fat++;
		if (fat >= 2 && fat <= 0xffefu &&
		    !mb_fat_valid_cluster(fat))
			report->bad_fat++;
	}

	if (report && (report->bad_bpb || report->bad_dirent ||
		       report->bad_chain || report->bad_fat ||
		       report->lost_clusters || report->cross_links))
		return MB_ERR_ACCDN;

	return 0;
}
