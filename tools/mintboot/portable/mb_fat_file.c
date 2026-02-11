#include "mb_fat_internal.h"

#include <string.h>

struct mb_fat_open *mb_fat_get_open(uint16_t handle)
{
	uint16_t idx = handle - 3;

	if (handle < 3)
		return NULL;
	if (idx >= MB_FAT_MAX_OPEN)
		return NULL;
	if (!mb_fat_open[idx].in_use)
		return NULL;
	return &mb_fat_open[idx];
}

int mb_fat_cluster_for_offset(const struct mb_fat_open *op, uint32_t offset,
			      uint32_t *cluster_out, uint32_t *cluster_off_out)
{
	uint32_t cluster_size = mb_fat_vol->spc * mb_fat_vol->recsiz;
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

long mb_fat_fopen(const char *path, uint16_t mode)
{
	struct mb_fat_dirent ent;
	uint32_t idx;

	if (mode != 0)
		return -1;

	{
		int rc = mb_fat_find_path(2, path, &ent);
		if (rc != 0)
			return MB_ERR_FILNF;
	}
	if (ent.attr & MB_FAT_ATTR_DIR)
		return -1;

	for (idx = 0; idx < MB_FAT_MAX_OPEN; idx++) {
		if (!mb_fat_open[idx].in_use) {
			mb_fat_open[idx].in_use = 1;
			mb_fat_open[idx].dev = mb_fat_vol->dev;
			mb_fat_open[idx].size = ent.size;
			mb_fat_open[idx].start_cluster = ent.start_lo;
			mb_fat_open[idx].offset = 0;
			mb_fat_open[idx].time = ent.wr_time;
			mb_fat_open[idx].date = ent.wr_date;
			mb_fat_open[idx].attr = ent.attr;
			return (long)(idx + 3);
		}
	}

	return MB_ERR_NHNDL;
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
		return MB_ERR_IHNDL;

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
		uint32_t cluster_size = mb_fat_vol->spc * mb_fat_vol->recsiz;
		uint32_t sector_in_cluster;
		uint32_t sector_off;
		uint32_t sector_idx;
		uint32_t chunk;

		if (mb_fat_cluster_for_offset(op, op->offset, &cluster,
					      &cluster_off) != 0)
			break;

		sector_in_cluster = cluster_off / mb_fat_vol->recsiz;
		sector_off = cluster_off % mb_fat_vol->recsiz;
		sector_idx = mb_fat_cluster_sector(cluster) + sector_in_cluster;

		if (mb_fat_rwabs(0, sector, 1, sector_idx,
				 mb_fat_vol->dev) != 0)
			break;

		chunk = mb_fat_vol->recsiz - sector_off;
		if (chunk > remaining)
			chunk = remaining;

		memcpy(dst, &sector[sector_off], chunk);
		dst += chunk;
		op->offset += chunk;
		read_total += chunk;
		remaining -= chunk;

		if (op->offset >= op->size)
			break;

		if (sector_off + chunk < mb_fat_vol->recsiz)
			continue;

		if ((sector_in_cluster + 1) >= mb_fat_vol->spc) {
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
		return MB_ERR_IHNDL;

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
		return MB_ERR_INVFN;
	}

	newpos = base + where;
	if (newpos < 0 || newpos > (int32_t)op->size)
		return MB_ERR_RANGE;
	op->offset = (uint32_t)newpos;
	return (long)op->offset;
}

long mb_fat_fclose(uint16_t handle)
{
	struct mb_fat_open *op;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_IHNDL;

	op->in_use = 0;
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
		return MB_ERR_IHNDL;

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

	{
		int rc = mb_fat_find_path(2, fn, &ent);
		if (rc != 0)
			return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : MB_ERR_FILNF;
	}

	return (long)ent.attr;
}

long mb_fat_dfree(uint32_t buf, uint16_t d)
{
	uint32_t cluster;
	uint32_t free_clusters = 0;
	uint16_t *out;

	if (mb_fat_mount(d) != 0)
		return -1;

	for (cluster = 2; cluster < (uint32_t)mb_fat_vol->total_clusters + 2u;
	     cluster++) {
		if (mb_fat_read_fat(cluster) == 0)
			free_clusters++;
	}

	out = (uint16_t *)(uintptr_t)buf;
	out[0] = (uint16_t)free_clusters;
	out[1] = (uint16_t)mb_fat_vol->total_clusters;
	out[2] = (uint16_t)mb_fat_vol->recsiz;
	out[3] = (uint16_t)mb_fat_vol->spc;
	return 0;
}

long mb_fat_dcreate(const char *path)
{
	uint16_t dev;
	uint32_t dir_cluster;
	uint8_t name83[11];
	uint32_t idx;
	struct mb_fat_dirent ent;
	uint8_t raw[32];
	uint32_t new_cluster;

	{
		int rc = mb_fat_locate_parent(path, &dev, &dir_cluster, name83);
		if (rc != 0)
			return rc;
	}

	if (mb_fat_mount(dev) != 0)
		return MB_ERR_ACCDN;

	idx = 0;
	if (mb_fat_find_in_dir(dir_cluster, name83, 0xffffu, &ent, &idx) == 0)
		return MB_ERR_ACCDN;

	if (mb_fat_find_free_entry(dir_cluster, &idx) != 0)
		return MB_ERR_ACCDN;

	new_cluster = mb_fat_alloc_cluster();
	if (new_cluster == 0)
		return MB_ERR_ACCDN;

	if (mb_fat_zero_cluster(new_cluster) != 0) {
		mb_fat_free_chain(new_cluster);
		return MB_ERR_ACCDN;
	}

	memset(raw, 0, sizeof(raw));
	memcpy(raw, name83, 11u);
	raw[11] = MB_FAT_ATTR_DIR;
	raw[26] = (uint8_t)(new_cluster & 0xff);
	raw[27] = (uint8_t)(new_cluster >> 8);
	if (mb_fat_write_dirent_raw(dir_cluster, idx, raw) != 0) {
		mb_fat_free_chain(new_cluster);
		return MB_ERR_ACCDN;
	}

	memset(raw, 0, sizeof(raw));
	raw[0] = '.';
	memset(&raw[1], ' ', 10);
	raw[11] = MB_FAT_ATTR_DIR;
	raw[26] = (uint8_t)(new_cluster & 0xff);
	raw[27] = (uint8_t)(new_cluster >> 8);
	if (mb_fat_write_dirent_raw(new_cluster, 0, raw) != 0) {
		mb_fat_free_chain(new_cluster);
		return MB_ERR_ACCDN;
	}

	memset(raw, 0, sizeof(raw));
	raw[0] = '.';
	raw[1] = '.';
	memset(&raw[2], ' ', 9);
	raw[11] = MB_FAT_ATTR_DIR;
	raw[26] = (uint8_t)(dir_cluster & 0xff);
	raw[27] = (uint8_t)(dir_cluster >> 8);
	if (mb_fat_write_dirent_raw(new_cluster, 1, raw) != 0) {
		mb_fat_free_chain(new_cluster);
		return MB_ERR_ACCDN;
	}

	return 0;
}

long mb_fat_ddelete(const char *path)
{
	uint16_t dev;
	uint32_t dir_cluster;
	uint8_t name83[11];
	struct mb_fat_dirent ent;
	uint32_t idx = 0;
	int rc;
	uint8_t raw[32];

	rc = mb_fat_locate_parent(path, &dev, &dir_cluster, name83);
	if (rc != 0)
		return rc;

	if (mb_fat_mount(dev) != 0)
		return MB_ERR_FILNF;

	if (mb_fat_find_in_dir(dir_cluster, name83, 0xffffu, &ent, &idx) != 0)
		return MB_ERR_FILNF;

	if (!(ent.attr & MB_FAT_ATTR_DIR))
		return MB_ERR_ACCDN;

	if (ent.attr & MB_FAT_ATTR_RDONLY)
		return MB_ERR_ACCDN;

	if (ent.start_lo < 2)
		return MB_ERR_ACCDN;

	rc = mb_fat_dir_is_empty(ent.start_lo);
	if (rc <= 0)
		return (rc == 0) ? MB_ERR_ACCDN : MB_ERR_FILNF;

	if (mb_fat_read_dirent_raw(dir_cluster, idx, raw) != 0)
		return MB_ERR_FILNF;
	raw[0] = 0xe5;
	if (mb_fat_write_dirent_raw(dir_cluster, idx, raw) != 0)
		return MB_ERR_FILNF;

	if (mb_fat_free_chain(ent.start_lo) != 0)
		return MB_ERR_ACCDN;

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
		return MB_ERR_NSAME;
	{
		int rc;
		rc = mb_fat_locate_parent(newname, &dev_dst, &dir_dst, name_dst);
		if (rc != 0)
			return rc;
	}

	if (mb_fat_mount(dev_src) != 0)
		return MB_ERR_FILNF;

	if (mb_fat_find_in_dir(dir_src, name_src, 0xffffu, &ent, &idx_src) != 0)
		return MB_ERR_FILNF;

	if (ent.attr & MB_FAT_ATTR_RDONLY)
		return MB_ERR_ACCDN;

	if (mb_fat_find_in_dir(dir_dst, name_dst, 0xffffu, &ent, &idx_dst) == 0)
		return MB_ERR_ACCDN;

	if (mb_fat_read_dirent_raw(dir_src, idx_src, raw) != 0)
		return MB_ERR_FILNF;

	memcpy(raw, name_dst, 11u);

	if (dir_src == dir_dst) {
		if (mb_fat_write_dirent_raw(dir_src, idx_src, raw) != 0)
			return -1;
		return 0;
	}

	if (mb_fat_find_free_entry(dir_dst, &idx_dst) != 0)
		return MB_ERR_FILNF;
	if (mb_fat_write_dirent_raw(dir_dst, idx_dst, raw) != 0)
		return -1;
	raw[0] = 0xe5;
	if (mb_fat_write_dirent_raw(dir_src, idx_src, raw) != 0)
		return -1;

	return 0;
}
