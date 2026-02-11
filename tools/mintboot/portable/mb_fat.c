#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * FAT16 filesystem (portable layer)
 *
 * - Initially read-only.
 * - Will grow to support read/write.
 * - Multiple concurrent open files supported.
 * - Not re-entrant or interrupt-safe.
 */

#define MB_FAT_MAX_SEARCH 4

#define MB_FAT_ATTR_RDONLY 0x01
#define MB_FAT_ATTR_HIDDEN 0x02
#define MB_FAT_ATTR_SYSTEM 0x04
#define MB_FAT_ATTR_LABEL  0x08
#define MB_FAT_ATTR_DIR    0x10
#define MB_FAT_ATTR_LFN    0x0f

#define MB_FAT_DTA_VALID 0x1234fedcUL
struct mb_fat_bpb {
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

struct mb_fat_volume {
	uint8_t valid;
	uint8_t dev;
	uint16_t recsiz;
	uint16_t spc;
	uint16_t num_fats;
	uint32_t fat_start;
	uint32_t root_start;
	uint32_t root_sectors;
	uint32_t data_start;
	uint32_t total_clusters;
};

struct mb_fat_dirent {
	uint8_t name[11];
	uint8_t attr;
	uint8_t ntres;
	uint8_t crt_tenths;
	uint16_t crt_time;
	uint16_t crt_date;
	uint16_t acc_date;
	uint16_t start_hi;
	uint16_t wr_time;
	uint16_t wr_date;
	uint16_t start_lo;
	uint32_t size;
} __attribute__((packed));

struct mb_fat_search {
	uint8_t in_use;
	uint8_t dev;
	uint8_t pattern[11];
	uint16_t attr;
	uint32_t dir_cluster;
	uint32_t entry_index;
};

struct mb_fat_open {
	uint8_t in_use;
	uint8_t locked;
	uint8_t dev;
	uint32_t size;
	uint32_t start_cluster;
	uint32_t offset;
	uint16_t time;
	uint16_t date;
	uint8_t attr;
};

struct mb_fat_dta {
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

static struct mb_fat_volume mb_fat_vol;
static struct mb_fat_search mb_fat_search[MB_FAT_MAX_SEARCH];
static struct mb_fat_open mb_fat_open[MB_FAT_MAX_OPEN];

static uint16_t mb_fat_le16(const uint8_t *ptr)
{
	return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

static uint32_t mb_fat_le32(const uint8_t *ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
	       ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}




static int mb_fat_rwabs(uint16_t rw, void *buf, uint16_t count,
			uint32_t recno, uint16_t dev)
{
	if (recno > 0xffffu)
		return -1;
	return (int)mb_rom_dispatch.rwabs(rw, buf, count, (uint16_t)recno, dev);
}

static int mb_fat_mount(uint16_t dev)
{
	const struct mb_fat_bpb *bpb;
	long bpb_ptr;
	uint16_t num_fats;
	uint32_t root_start;
	uint32_t fat_start;

	if (mb_fat_vol.valid && mb_fat_vol.dev == dev)
		return 0;

	bpb_ptr = mb_rom_dispatch.getbpb(dev);
	if (bpb_ptr <= 0)
		return -1;

	bpb = (const struct mb_fat_bpb *)(uintptr_t)bpb_ptr;
	if (!bpb)
		return -1;

	if (bpb->recsiz != 512 || bpb->clsiz <= 0 || bpb->fsiz <= 0 ||
	    bpb->rdlen < 0 || bpb->datrec <= 0)
		return -1;

	num_fats = (bpb->bflags & (1 << 1)) ? 1 : 2;
	root_start = (uint32_t)bpb->datrec - (uint32_t)bpb->rdlen;
	fat_start = root_start - ((uint32_t)bpb->fsiz * num_fats);

	mb_fat_vol.valid = 1;
	mb_fat_vol.dev = (uint8_t)dev;
	mb_fat_vol.recsiz = bpb->recsiz;
	mb_fat_vol.spc = (uint16_t)bpb->clsiz;
	mb_fat_vol.num_fats = num_fats;
	mb_fat_vol.fat_start = fat_start;
	mb_fat_vol.root_start = root_start;
	mb_fat_vol.root_sectors = (uint32_t)bpb->rdlen;
	mb_fat_vol.data_start = (uint32_t)bpb->datrec;
	mb_fat_vol.total_clusters = (uint32_t)bpb->numcl;
	return 0;
}

static uint32_t mb_fat_cluster_sector(uint32_t cluster)
{
	if (cluster < 2)
		return 0;
	return mb_fat_vol.data_start +
	       ((cluster - 2u) * mb_fat_vol.spc);
}

static uint16_t mb_fat_read_fat(uint32_t cluster)
{
	uint8_t sector[512];
	uint32_t offset = cluster * 2u;
	uint32_t sector_idx = offset / mb_fat_vol.recsiz;
	uint32_t sector_off = offset % mb_fat_vol.recsiz;

	if (mb_fat_rwabs(0, sector, 1,
			 mb_fat_vol.fat_start + sector_idx,
			 mb_fat_vol.dev) != 0)
		return 0xffffu;

	return mb_fat_le16(&sector[sector_off]);
}


static void mb_fat_pattern_83(const char *pattern, uint8_t out[11])
{
	uint8_t i = 0;
	uint8_t o = 0;
	int saw_dot = 0;

	for (; o < 11; o++)
		out[o] = ' ';

	o = 0;
	while (pattern[i] && o < 11) {
		char c = pattern[i++];
		if (c == '/' || c == '\\')
			break;
		if (c == '*') {
			if (!saw_dot) {
				while (o < 8)
					out[o++] = '?';
			} else {
				while (o < 11)
					out[o++] = '?';
			}
			continue;
		}
		if (c == '.') {
			saw_dot = 1;
			o = 8;
			continue;
		}
		if (c == '?') {
			out[o++] = '?';
			continue;
		}
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 'a' + 'A');
		out[o++] = (uint8_t)c;
	}
}

static int mb_fat_match_83(const uint8_t name[11], const uint8_t pat[11])
{
	uint8_t i;

	for (i = 0; i < 11; i++) {
		if (pat[i] == '?')
			continue;
		if (pat[i] != name[i])
			return 0;
	}
	return 1;
}

static void mb_fat_name_from_dirent(const struct mb_fat_dirent *ent,
				    char out[14])
{
	uint8_t i = 0;
	uint8_t o = 0;

	for (; i < 8; i++) {
		if (ent->name[i] == ' ')
			break;
		out[o++] = (char)ent->name[i];
	}
	if (ent->name[8] != ' ' || ent->name[9] != ' ' || ent->name[10] != ' ') {
		out[o++] = '.';
		for (i = 8; i < 11; i++) {
			if (ent->name[i] == ' ')
				break;
			out[o++] = (char)ent->name[i];
		}
	}
	out[o] = '\0';
}

static int mb_fat_has_wildcards(const char *name)
{
	for (; *name && *name != '\\' && *name != '/'; name++) {
		if (*name == '*' || *name == '?')
			return 1;
	}
	return 0;
}

static int mb_fat_dirent_location(uint32_t dir_cluster, uint32_t index,
				  uint32_t *sector_idx, uint32_t *sector_off)
{
	uint32_t entries_per_sector = mb_fat_vol.recsiz / 32u;
	uint32_t cluster = dir_cluster;
	uint32_t entry_idx = index;

	if (mb_fat_vol.recsiz != 512)
		return -1;

	if (dir_cluster == 0) {
		uint32_t total_entries = (mb_fat_vol.root_sectors *
					  entries_per_sector);
		if (index >= total_entries)
			return -1;
		*sector_idx = mb_fat_vol.root_start +
			      (index / entries_per_sector);
		*sector_off = (index % entries_per_sector) * 32u;
		return 0;
	}

	while (entry_idx >= (mb_fat_vol.spc * entries_per_sector)) {
		uint16_t next = mb_fat_read_fat(cluster);
		if (next >= 0xfff8u)
			return -1;
		cluster = next;
		entry_idx -= mb_fat_vol.spc * entries_per_sector;
	}

	*sector_idx = mb_fat_cluster_sector(cluster) +
		      (entry_idx / entries_per_sector);
	*sector_off = (entry_idx % entries_per_sector) * 32u;
	return 0;
}

static int mb_fat_read_dirent_raw(uint32_t dir_cluster, uint32_t index,
				  uint8_t raw[32])
{
	uint8_t sector[512];
	uint32_t sector_idx;
	uint32_t sector_off;

	if (mb_fat_dirent_location(dir_cluster, index, &sector_idx,
				   &sector_off) != 0)
		return -1;
	if (mb_fat_rwabs(0, sector, 1, sector_idx,
			 mb_fat_vol.dev) != 0)
		return -1;
	memcpy(raw, &sector[sector_off], 32u);
	return 0;
}

static void mb_fat_decode_dirent(const uint8_t raw[32],
				 struct mb_fat_dirent *out)
{
	memcpy(out->name, raw, 11u);
	out->attr = raw[11];
	out->ntres = raw[12];
	out->crt_tenths = raw[13];
	out->crt_time = mb_fat_le16(&raw[14]);
	out->crt_date = mb_fat_le16(&raw[16]);
	out->acc_date = mb_fat_le16(&raw[18]);
	out->start_hi = mb_fat_le16(&raw[20]);
	out->wr_time = mb_fat_le16(&raw[22]);
	out->wr_date = mb_fat_le16(&raw[24]);
	out->start_lo = mb_fat_le16(&raw[26]);
	out->size = mb_fat_le32(&raw[28]);
}

static int mb_fat_write_dirent_raw(uint32_t dir_cluster, uint32_t index,
				   const uint8_t raw[32])
{
	uint8_t sector[512];
	uint32_t sector_idx;
	uint32_t sector_off;

	if (mb_fat_dirent_location(dir_cluster, index, &sector_idx,
				   &sector_off) != 0)
		return -1;
	if (mb_fat_rwabs(0, sector, 1, sector_idx,
			 mb_fat_vol.dev) != 0)
		return -1;
	memcpy(&sector[sector_off], raw, 32u);
	if (mb_fat_rwabs(1, sector, 1, sector_idx,
			 mb_fat_vol.dev) != 0)
		return -1;
	return 0;
}

static int mb_fat_read_dirent(uint32_t dir_cluster, uint32_t index,
			      struct mb_fat_dirent *out)
{
	uint8_t sector[512];
	uint8_t raw[32];
	uint32_t sector_idx;
	uint32_t sector_off;

	if (mb_fat_dirent_location(dir_cluster, index, &sector_idx,
				   &sector_off) != 0)
		return -1;
	if (mb_fat_rwabs(0, sector, 1, sector_idx,
			 mb_fat_vol.dev) != 0)
		return -1;
	memcpy(raw, &sector[sector_off], 32u);
	mb_fat_decode_dirent(raw, out);
	return 0;
}

static int mb_fat_find_in_dir(uint32_t dir_cluster, const uint8_t pat[11],
			      uint16_t attr, struct mb_fat_dirent *ent,
			      uint32_t *index)
{
	uint32_t idx = *index;
	struct mb_fat_dirent cur;
	uint8_t name83[11];

	for (;;) {
		if (mb_fat_read_dirent(dir_cluster, idx, &cur) != 0)
			return -1;

		if (cur.name[0] == 0x00)
			return -1;
		if (cur.name[0] == 0xe5 || cur.attr == MB_FAT_ATTR_LFN) {
			idx++;
			continue;
		}

		memcpy(name83, cur.name, 11);
		if (!mb_fat_match_83(name83, pat)) {
			idx++;
			continue;
		}

		if ((cur.attr & (MB_FAT_ATTR_HIDDEN | MB_FAT_ATTR_SYSTEM |
				 MB_FAT_ATTR_DIR | MB_FAT_ATTR_LABEL)) &&
		    !(attr & cur.attr)) {
			idx++;
			continue;
		}

		memcpy((uint8_t *)ent, (const uint8_t *)&cur,
		       (uint32_t)sizeof(cur));
		*index = idx;
		return 0;
	}
}

static int mb_fat_find_path(uint16_t dev, const char *path,
			    struct mb_fat_dirent *ent)
{
	const char *p = path;
	char part[64];
	uint8_t pat[11];
	uint32_t dir_cluster = 0;

	if (!path)
		return -1;

	if (p[1] == ':') {
		char drive = p[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		dev = (uint16_t)(drive - 'A');
		p += 2;
	}

	while (*p == '\\' || *p == '/')
		p++;

	while (*p) {
		uint32_t i = 0;
		while (*p && *p != '\\' && *p != '/' && i < sizeof(part) - 1)
			part[i++] = *p++;
		part[i] = '\0';

		while (*p == '\\' || *p == '/')
			p++;

		if (mb_fat_mount(dev) != 0)
			return -1;

		mb_fat_pattern_83(part, pat);

		if (*p) {
			uint32_t idx = 0;
			struct mb_fat_dirent dirent;
			if (mb_fat_find_in_dir(dir_cluster, pat, MB_FAT_ATTR_DIR,
					       &dirent, &idx) != 0)
				return -1;
			if (!(dirent.attr & MB_FAT_ATTR_DIR))
				return -1;
			dir_cluster = dirent.start_lo;
			continue;
		}

		{
			uint32_t idx = 0;
			if (mb_fat_find_in_dir(dir_cluster, pat, 0xffffu,
					       ent, &idx) != 0)
				return -1;
			return 0;
		}
	}

	return -1;
}

static int mb_fat_setup_search(const char *filespec, uint16_t attr,
			       struct mb_fat_search **out)
{
	const char *p = filespec;
	char part[64];
	uint8_t pat[11];
	uint16_t dev = 2;
	uint32_t dir_cluster = 0;
	uint32_t idx;

	if (!filespec)
		return -1;

	if (p[1] == ':') {
		char drive = p[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		dev = (uint16_t)(drive - 'A');
		p += 2;
	}

	while (*p == '\\' || *p == '/')
		p++;

	for (;;) {
		uint32_t i = 0;
		while (*p && *p != '\\' && *p != '/' && i < sizeof(part) - 1)
			part[i++] = *p++;
		part[i] = '\0';

		while (*p == '\\' || *p == '/')
			p++;

		if (mb_fat_mount(dev) != 0)
			return -1;

		mb_fat_pattern_83(part[0] ? part : "*", pat);

		if (*p) {
			uint32_t search_idx = 0;
			struct mb_fat_dirent dirent;
			if (mb_fat_find_in_dir(dir_cluster, pat, MB_FAT_ATTR_DIR,
					       &dirent, &search_idx) != 0)
				return -1;
			if (!(dirent.attr & MB_FAT_ATTR_DIR))
				return -1;
			dir_cluster = dirent.start_lo;
			continue;
		}

		break;
	}

	for (idx = 0; idx < MB_FAT_MAX_SEARCH; idx++) {
		if (!mb_fat_search[idx].in_use) {
			mb_fat_search[idx].in_use = 1;
			mb_fat_search[idx].dev = (uint8_t)dev;
			memcpy(mb_fat_search[idx].pattern, pat, 11);
			mb_fat_search[idx].attr = attr;
			mb_fat_search[idx].dir_cluster = dir_cluster;
			mb_fat_search[idx].entry_index = 0;
			*out = &mb_fat_search[idx];
			return 0;
		}
	}

	return -1;
}

static int mb_fat_locate_parent(const char *path, uint16_t *dev_out,
				uint32_t *dir_cluster_out, uint8_t name83[11])
{
	const char *p = path;
	char part[64];
	uint16_t dev = 2;
	uint32_t dir_cluster = 0;

	if (!path)
		return MB_ERR_PTH;

	if (p[1] == ':') {
		char drive = p[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		dev = (uint16_t)(drive - 'A');
		p += 2;
	}

	while (*p == '\\' || *p == '/')
		p++;

	for (;;) {
		uint32_t i = 0;
		while (*p && *p != '\\' && *p != '/' && i < sizeof(part) - 1)
			part[i++] = *p++;
		part[i] = '\0';

		while (*p == '\\' || *p == '/')
			p++;

		if (mb_fat_mount(dev) != 0)
			return MB_ERR_FNF;

		if (!*p) {
			if (part[0] == '\0')
				return MB_ERR_PTH;
			if (mb_fat_has_wildcards(part))
				return MB_ERR_FNF;
			mb_fat_pattern_83(part, name83);
			*dev_out = dev;
			*dir_cluster_out = dir_cluster;
			return 0;
		}

		{
			struct mb_fat_dirent dirent;
			uint8_t pat[11];
			uint32_t search_idx = 0;

			if (mb_fat_has_wildcards(part))
				return MB_ERR_PTH;
			mb_fat_pattern_83(part, pat);
			if (mb_fat_find_in_dir(dir_cluster, pat, MB_FAT_ATTR_DIR,
					       &dirent, &search_idx) != 0)
				return MB_ERR_PTH;
			if (!(dirent.attr & MB_FAT_ATTR_DIR))
				return MB_ERR_PTH;
			dir_cluster = dirent.start_lo;
		}
	}
}

static int mb_fat_parse_drive(const char *path, uint16_t *dev_out)
{
	uint16_t dev = 2;

	if (!path)
		return MB_ERR_PTH;

	if (path[1] == ':') {
		char drive = path[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		if (drive < 'A' || drive > 'Z')
			return MB_ERR_PTH;
		dev = (uint16_t)(drive - 'A');
	}

	*dev_out = dev;
	return 0;
}

static int mb_fat_find_free_entry(uint32_t dir_cluster, uint32_t *index)
{
	uint32_t idx = 0;
	uint8_t raw[32];

	for (;;) {
		if (mb_fat_read_dirent_raw(dir_cluster, idx, raw) != 0)
			return -1;
		if (raw[0] == 0x00 || raw[0] == 0xe5) {
			*index = idx;
			return 0;
		}
		idx++;
	}
}

long mb_fat_fsfirst(const char *filespec, uint16_t attr)
{
	struct mb_fat_search *search;
	struct mb_fat_dirent ent;
	struct mb_fat_dta *dta;
	uint32_t idx;
	char name[14];

	dta = (struct mb_fat_dta *)mb_rom_fgetdta();
	if (!dta)
		return -1;

	if (mb_fat_setup_search(filespec, attr, &search) != 0)
		return MB_ERR_FNF;

	idx = search->entry_index;
	if (mb_fat_find_in_dir(search->dir_cluster, search->pattern, search->attr,
			       &ent, &idx) != 0) {
		search->in_use = 0;
		return MB_ERR_FNF;
	}

	search->entry_index = idx + 1;

	dta->index = (uint16_t)(search - mb_fat_search);
	dta->magic = MB_FAT_DTA_VALID;
	dta->dta_sattrib = (char)attr;
	dta->dta_attrib = (char)ent.attr;
	dta->dta_time = ent.wr_time;
	dta->dta_date = ent.wr_date;
	dta->dta_size = ent.size;
	mb_fat_name_from_dirent(&ent, name);
	memset((uint8_t *)dta->dta_name, 0, sizeof(dta->dta_name));
	memcpy((uint8_t *)dta->dta_name, (const uint8_t *)name,
	       (uint32_t)(sizeof(dta->dta_name) - 1));
	return 0;
}

long mb_fat_fsnext(void)
{
	struct mb_fat_search *search;
	struct mb_fat_dirent ent;
	struct mb_fat_dta *dta;
	uint32_t idx;
	char name[14];

	dta = (struct mb_fat_dta *)mb_rom_fgetdta();
	if (!dta || dta->magic != MB_FAT_DTA_VALID)
		return MB_ERR_FNF;

	if (dta->index >= MB_FAT_MAX_SEARCH)
		return MB_ERR_FNF;
	search = &mb_fat_search[dta->index];
	if (!search->in_use)
		return MB_ERR_FNF;

	idx = search->entry_index;
	if (mb_fat_find_in_dir(search->dir_cluster, search->pattern, search->attr,
			       &ent, &idx) != 0) {
		search->in_use = 0;
		return MB_ERR_NMFIL;
	}

	search->entry_index = idx + 1;
	dta->dta_attrib = (char)ent.attr;
	dta->dta_time = ent.wr_time;
	dta->dta_date = ent.wr_date;
	dta->dta_size = ent.size;
	mb_fat_name_from_dirent(&ent, name);
	memset((uint8_t *)dta->dta_name, 0, sizeof(dta->dta_name));
	memcpy((uint8_t *)dta->dta_name, (const uint8_t *)name,
	       (uint32_t)(sizeof(dta->dta_name) - 1));
	return 0;
}

long mb_fat_fopen(const char *path, uint16_t mode)
{
	struct mb_fat_dirent ent;
	uint32_t idx;

	if (mode != 0)
		return -1;

	if (mb_fat_find_path(2, path, &ent) != 0)
		return MB_ERR_FNF;
	if (ent.attr & MB_FAT_ATTR_DIR)
		return -1;

	for (idx = 0; idx < MB_FAT_MAX_OPEN; idx++) {
		if (!mb_fat_open[idx].in_use) {
			mb_fat_open[idx].in_use = 1;
			mb_fat_open[idx].dev = mb_fat_vol.dev;
			mb_fat_open[idx].size = ent.size;
			mb_fat_open[idx].start_cluster = ent.start_lo;
			mb_fat_open[idx].offset = 0;
			mb_fat_open[idx].time = ent.wr_time;
			mb_fat_open[idx].date = ent.wr_date;
			mb_fat_open[idx].attr = ent.attr;
			mb_fat_open[idx].locked = 0;
			return (long)(idx + 3);
		}
	}

	return MB_ERR_NMFIL;
}

static struct mb_fat_open *mb_fat_get_open(uint16_t handle)
{
	uint32_t idx;

	if (handle < 3)
		return NULL;
	idx = (uint32_t)(handle - 3);
	if (idx >= MB_FAT_MAX_OPEN)
		return NULL;
	if (!mb_fat_open[idx].in_use)
		return NULL;
	return &mb_fat_open[idx];
}

static int mb_fat_cluster_for_offset(const struct mb_fat_open *op,
				     uint32_t offset,
				     uint32_t *cluster_out,
				     uint32_t *cluster_off_out)
{
	uint32_t cluster_size = mb_fat_vol.spc * mb_fat_vol.recsiz;
	uint32_t cluster_index;
	uint32_t cluster;
	uint32_t i;

	if (cluster_size == 0)
		return -1;

	cluster_index = offset / cluster_size;
	*cluster_off_out = offset % cluster_size;

	cluster = op->start_cluster;
	if (cluster < 2)
		return -1;

	for (i = 0; i < cluster_index; i++) {
		uint16_t next = mb_fat_read_fat(cluster);
		if (next >= 0xfff8u)
			return -1;
		cluster = next;
	}

	*cluster_out = cluster;
	return 0;
}

long mb_fat_fread(uint16_t handle, uint32_t cnt, void *buf)
{
	struct mb_fat_open *op;
	uint8_t sector[512];
	uint32_t remaining;
	uint32_t read_total;
	uint8_t *dst;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_BADF;

	if (op->locked)
		return MB_ERR_LOCKED;

	if (mb_fat_mount(op->dev) != 0)
		return -1;

	if (op->offset >= op->size)
		return 0;

	if (cnt > (op->size - op->offset))
		cnt = op->size - op->offset;

	remaining = cnt;
	read_total = 0;
	dst = (uint8_t *)buf;

	while (remaining > 0) {
		uint32_t cluster;
		uint32_t cluster_off;
		uint32_t cluster_size = mb_fat_vol.spc * mb_fat_vol.recsiz;
		uint32_t sector_in_cluster;
		uint32_t sector_off;
		uint32_t sector_idx;
		uint32_t chunk;

		if (mb_fat_cluster_for_offset(op, op->offset, &cluster,
					      &cluster_off) != 0)
			break;

		sector_in_cluster = cluster_off / mb_fat_vol.recsiz;
		sector_off = cluster_off % mb_fat_vol.recsiz;
		sector_idx = mb_fat_cluster_sector(cluster) + sector_in_cluster;

		if (mb_fat_rwabs(0, sector, 1, sector_idx,
				 mb_fat_vol.dev) != 0)
			break;

		chunk = mb_fat_vol.recsiz - sector_off;
		if (chunk > remaining)
			chunk = remaining;

		memcpy(dst, &sector[sector_off], chunk);
		dst += chunk;
		op->offset += chunk;
		read_total += chunk;
		remaining -= chunk;

		if (op->offset >= op->size)
			break;

		if (sector_off + chunk < mb_fat_vol.recsiz)
			continue;

		if ((sector_in_cluster + 1) >= mb_fat_vol.spc) {
			uint32_t next = mb_fat_read_fat(cluster);
			if (next >= 0xfff8u)
				break;
		}

		(void)cluster_size;
	}

	return (long)read_total;
}

long mb_fat_fseek(uint16_t handle, int32_t where, uint16_t how)
{
	struct mb_fat_open *op;
	int32_t base;
	int32_t newpos;

	op = mb_fat_get_open(handle);
	if (!op)
		return -1;

	switch (how) {
	case 0:
		base = 0;
		break;
	case 1:
		base = (int32_t)op->offset;
		break;
	case 2:
		base = (int32_t)op->size;
		break;
	default:
		return -1;
	}

	newpos = base + where;
	if (newpos < 0)
		return -1;
	op->offset = (uint32_t)newpos;
	return (long)op->offset;
}

long mb_fat_fclose(uint16_t handle)
{
	struct mb_fat_open *op;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_BADF;

	op->in_use = 0;
	op->locked = 0;
	return 0;
}

long mb_fat_fdatime(uint32_t timeptr, uint16_t handle, uint16_t rwflag)
{
	uint16_t *tp;
	struct mb_fat_open *op;

	if (rwflag != 0)
		return -1;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_BADF;

	tp = (uint16_t *)(uintptr_t)timeptr;
	tp[0] = op->time;
	tp[1] = op->date;
	return 0;
}

long mb_fat_fattrib(const char *fn, uint16_t rwflag, uint16_t attr)
{
	struct mb_fat_dirent ent;

	(void)attr;

	if (rwflag != 0)
		return MB_ERR_ACCDN;

	if (mb_fat_find_path(2, fn, &ent) != 0)
		return MB_ERR_FNF;

	return (long)ent.attr;
}

long mb_fat_dfree(uint32_t buf, uint16_t d)
{
	uint32_t cluster;
	uint32_t free_clusters = 0;
	uint16_t *out;

	if (mb_fat_mount(d) != 0)
		return -1;

	for (cluster = 2; cluster < (uint32_t)mb_fat_vol.total_clusters + 2u;
	     cluster++) {
		if (mb_fat_read_fat(cluster) == 0)
			free_clusters++;
	}

	out = (uint16_t *)(uintptr_t)buf;
	out[0] = (uint16_t)free_clusters;
	out[1] = (uint16_t)mb_fat_vol.total_clusters;
	out[2] = (uint16_t)mb_fat_vol.recsiz;
	out[3] = (uint16_t)mb_fat_vol.spc;
	return 0;
}

long mb_fat_frename(uint16_t zero, const char *oldname, const char *newname)
{
	uint16_t dev_src;
	uint16_t dev_dst;
	uint32_t dir_src;
	uint32_t dir_dst;
	uint8_t name_src[11];
	uint8_t name_dst[11];
	uint32_t idx_src = 0;
	uint32_t idx_dst = 0;
	struct mb_fat_dirent ent;
	uint8_t raw[32];

	(void)zero;

	{
		int rc;
		rc = mb_fat_locate_parent(oldname, &dev_src, &dir_src, name_src);
		if (rc != 0)
			return rc;
		rc = mb_fat_parse_drive(newname, &dev_dst);
		if (rc != 0)
			return rc;
	}

	if (dev_src != dev_dst)
		return MB_ERR_XDEV;
	{
		int rc;
		rc = mb_fat_locate_parent(newname, &dev_dst, &dir_dst, name_dst);
		if (rc != 0)
			return rc;
	}

	if (mb_fat_mount(dev_src) != 0)
		return MB_ERR_FNF;

	if (mb_fat_find_in_dir(dir_src, name_src, 0xffffu, &ent, &idx_src) != 0)
		return MB_ERR_FNF;

	if (mb_fat_find_in_dir(dir_dst, name_dst, 0xffffu, &ent, &idx_dst) == 0)
		return MB_ERR_EXIST;

	if (mb_fat_read_dirent_raw(dir_src, idx_src, raw) != 0)
		return MB_ERR_FNF;

	memcpy(raw, name_dst, 11u);

	if (dir_src == dir_dst) {
		if (mb_fat_write_dirent_raw(dir_src, idx_src, raw) != 0)
			return -1;
		return 0;
	}

	if (mb_fat_find_free_entry(dir_dst, &idx_dst) != 0)
		return MB_ERR_FNF;
	if (mb_fat_write_dirent_raw(dir_dst, idx_dst, raw) != 0)
		return -1;
	raw[0] = 0xe5;
	if (mb_fat_write_dirent_raw(dir_src, idx_src, raw) != 0)
		return -1;

	return 0;
}

long mb_fat_flock(uint16_t handle, uint16_t mode, int32_t start, int32_t length)
{
	struct mb_fat_open *op;

	(void)start;
	(void)length;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_BADF;

	op->locked = (mode != 0);
	return 0;
}
