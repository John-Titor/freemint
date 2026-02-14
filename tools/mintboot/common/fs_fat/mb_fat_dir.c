#include "mb_fat_internal.h"
#include "mintboot/mb_rom.h"

#include "mintboot/mb_lib.h"

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

static const char *mb_fat_resolve_path(uint16_t dev_in, const char *path,
				       uint16_t *dev_out, char *buf, size_t bufsz)
{
	const char *p = path;
	uint16_t dev = dev_in;
	const char *cur;
	size_t cur_len = 0;
	size_t out = 0;

	if (!path || !path[0])
		return NULL;

	if (p[1] == ':') {
		char drive = p[0];
		const char drive_max = (char)('A' + (MB_MAX_DRIVES - 1u));
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		if (drive < 'A' || drive > drive_max) {
			if (dev_out)
				*dev_out = 0xffffu;
			return NULL;
		}
		dev = (uint16_t)(drive - 'A');
		p += 2;
	} else {
		dev = mb_bdos_get_current_drive();
	}

	if (*p == '\\' || *p == '/') {
		if (dev_out)
			*dev_out = dev;
		return p;
	}

	cur = mb_bdos_get_current_path(dev);
	while (cur[cur_len])
		cur_len++;

	if (!buf || bufsz < 2)
		return NULL;
	if (cur_len == 0) {
		buf[out++] = '\\';
	} else {
		size_t i;
		for (i = 0; i < cur_len && out + 1 < bufsz; i++)
			buf[out++] = cur[i];
		if (out == 0 || buf[out - 1] != '\\')
			buf[out++] = '\\';
	}

	while (*p && out + 1 < bufsz) {
		char c = *p++;
		if (c == '/')
			c = '\\';
		buf[out++] = c;
	}
	buf[out] = '\0';
	if (dev_out)
		*dev_out = dev;
	return buf;
}

int mb_fat_find_path(uint16_t dev, const char *path, struct mb_fat_dirent *ent)
{
	char resolved[256];
	const char *p = mb_fat_resolve_path(dev, path, &dev, resolved, sizeof(resolved));
	char part[64];
	uint8_t pat[11];
	uint32_t dir_cluster = 0;

	if (!p)
		return (dev == 0xffffu) ? MB_ERR_DRIVE : -1;

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
	char resolved[256];
	uint16_t dev = mb_bdos_get_current_drive();
	const char *p = mb_fat_resolve_path(dev, filespec, &dev, resolved, sizeof(resolved));
	char part[64];
	uint8_t pat[11];
	uint32_t dir_cluster = 0;
	uint32_t idx;

	if (!p)
		return (dev == 0xffffu) ? MB_ERR_DRIVE : -1;

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
	char part[64];
	uint16_t dev = mb_bdos_get_current_drive();
	char resolved[256];
	const char *p = mb_fat_resolve_path(dev, path, &dev, resolved, sizeof(resolved));
	uint32_t dir_cluster = 0;

	if (!p)
		return (dev == 0xffffu) ? MB_ERR_DRIVE : MB_ERR_PTHNF;

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
