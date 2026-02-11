#include "mb_fat_internal.h"
#include "mintboot/mb_rom.h"

#include <string.h>

int mb_fat_find_in_dir(uint32_t dir_cluster, const uint8_t pat[11],
		      uint16_t attr, struct mb_fat_dirent *ent, uint32_t *index)
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

int mb_fat_dir_is_empty(uint32_t dir_cluster)
{
	uint32_t idx = 0;
	uint8_t raw[32];

	for (;;) {
		if (mb_fat_read_dirent_raw(dir_cluster, idx, raw) != 0)
			return -1;
		if (raw[0] == 0x00)
			return 1;
		if (raw[0] == 0xe5) {
			idx++;
			continue;
		}
		if (raw[11] == MB_FAT_ATTR_LFN) {
			idx++;
			continue;
		}
		if (raw[0] == '.' && (raw[1] == ' ' || raw[1] == '.')) {
			idx++;
			continue;
		}
		return 0;
	}
}

int mb_fat_find_path(uint16_t dev, const char *path, struct mb_fat_dirent *ent)
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
		if (drive < 'A' || drive > 'Z')
			return MB_ERR_DRIVE;
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

		{
			int rc = mb_fat_mount(dev);
			if (rc != 0)
				return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : -1;
		}

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

int mb_fat_setup_search(const char *filespec, uint16_t attr,
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
		if (drive < 'A' || drive > 'Z')
			return MB_ERR_DRIVE;
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

		{
			int rc = mb_fat_mount(dev);
			if (rc != 0)
				return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : -1;
		}

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

int mb_fat_locate_parent(const char *path, uint16_t *dev_out,
			  uint32_t *dir_cluster_out, uint8_t name83[11])
{
	const char *p = path;
	char part[64];
	uint16_t dev = 2;
	uint32_t dir_cluster = 0;

	if (!path)
		return MB_ERR_PTHNF;

	if (p[1] == ':') {
		char drive = p[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		if (drive < 'A' || drive > 'Z')
			return MB_ERR_DRIVE;
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

		{
			int rc = mb_fat_mount(dev);
			if (rc != 0)
				return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : MB_ERR_FILNF;
		}

		if (!*p) {
			if (part[0] == '\0')
				return MB_ERR_PTHNF;
			if (mb_fat_has_wildcards(part))
				return MB_ERR_FILNF;
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
				return MB_ERR_PTHNF;
			mb_fat_pattern_83(part, pat);
			if (mb_fat_find_in_dir(dir_cluster, pat, MB_FAT_ATTR_DIR,
					       &dirent, &search_idx) != 0)
				return MB_ERR_PTHNF;
			if (!(dirent.attr & MB_FAT_ATTR_DIR))
				return MB_ERR_PTHNF;
			dir_cluster = dirent.start_lo;
		}
	}
}

int mb_fat_find_free_entry(uint32_t dir_cluster, uint32_t *index)
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

	{
		int rc = mb_fat_setup_search(filespec, attr, &search);
		if (rc != 0)
			return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : MB_ERR_FILNF;
	}

	idx = search->entry_index;
	if (mb_fat_find_in_dir(search->dir_cluster, search->pattern, search->attr,
			       &ent, &idx) != 0) {
		search->in_use = 0;
		return MB_ERR_FILNF;
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
		return MB_ERR_FILNF;

	if (dta->index >= MB_FAT_MAX_SEARCH)
		return MB_ERR_FILNF;
	search = &mb_fat_search[dta->index];
	if (!search->in_use)
		return MB_ERR_FILNF;

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
