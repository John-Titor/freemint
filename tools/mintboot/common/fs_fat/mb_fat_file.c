#include "mb_fat_internal.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_util.h"
#include "mintboot/mb_common.h"

#include <string.h>

static int mb_fat_find_free_open_slot(uint32_t *idx_out)
{
	uint32_t idx;

	for (idx = 0; idx < MB_FAT_MAX_OPEN; idx++) {
		if (!mb_fat_open[idx].in_use) {
			*idx_out = idx;
			return 0;
		}
	}

	return -1;
}

static int mb_fat_sync_open_dirent(const struct mb_fat_open *op)
{
	uint8_t raw[32];

	if (mb_fat_mount(op->dev) != 0)
		return -1;
	if (mb_fat_read_dirent_raw(op->dir_cluster, op->dir_index, raw) != 0)
		return -1;
	if (raw[0] == 0x00 || raw[0] == 0xe5)
		return -1;

	raw[11] = (uint8_t)((raw[11] & MB_FAT_ATTR_DIR) |
			    (op->attr & (MB_FAT_ATTR_RDONLY |
					 MB_FAT_ATTR_HIDDEN |
					 MB_FAT_ATTR_SYSTEM |
					 MB_FAT_ATTR_ARCH)));
	raw[22] = (uint8_t)(op->time & 0xff);
	raw[23] = (uint8_t)(op->time >> 8);
	raw[24] = (uint8_t)(op->date & 0xff);
	raw[25] = (uint8_t)(op->date >> 8);
	raw[26] = (uint8_t)(op->start_cluster & 0xff);
	raw[27] = (uint8_t)(op->start_cluster >> 8);
	raw[28] = (uint8_t)(op->size & 0xff);
	raw[29] = (uint8_t)((op->size >> 8) & 0xff);
	raw[30] = (uint8_t)((op->size >> 16) & 0xff);
	raw[31] = (uint8_t)((op->size >> 24) & 0xff);

	return mb_fat_write_dirent_raw(op->dir_cluster, op->dir_index, raw);
}

static int mb_fat_ensure_cluster(struct mb_fat_open *op, uint32_t cluster_index,
				 uint32_t *cluster_out)
{
	uint32_t cluster = op->start_cluster;
	uint32_t i;

	if (cluster < 2u) {
		cluster = mb_fat_alloc_cluster();
		if (cluster < 2u)
			return -1;
		if (mb_fat_zero_cluster(cluster) != 0) {
			mb_fat_free_chain(cluster);
			return -1;
		}
		op->start_cluster = cluster;
		op->dirty = 1;
	}

	for (i = 0; i < cluster_index; i++) {
		uint16_t next = mb_fat_read_fat(cluster);
		if (next >= 0xfff8u) {
			uint32_t new_cluster = mb_fat_alloc_cluster();
			if (new_cluster < 2u)
				return -1;
			if (mb_fat_zero_cluster(new_cluster) != 0) {
				mb_fat_free_chain(new_cluster);
				return -1;
			}
			if (mb_fat_write_fat(cluster, (uint16_t)new_cluster) != 0) {
				mb_fat_free_chain(new_cluster);
				return -1;
			}
			cluster = new_cluster;
			op->dirty = 1;
		} else {
			cluster = next;
		}
	}

	*cluster_out = cluster;
	return 0;
}

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
	uint16_t dev;
	uint32_t dir_cluster;
	uint8_t name83[11];
	uint32_t dir_index = 0;
	uint32_t idx;

	if (mode != 0)
		return -1;

	if (mb_fat_locate_parent(path, &dev, &dir_cluster, name83) != 0) {
		mb_log_printf("fat_fopen: locate_parent fail path='%s'\n",
			      mb_guarded_str(path));
		return MB_ERR_FILNF;
	}
	if (mb_fat_mount(dev) != 0) {
		mb_log_printf("fat_fopen: mount fail dev=%u path='%s'\n",
			      (uint32_t)dev, mb_guarded_str(path));
		return MB_ERR_FILNF;
	}
	if (mb_fat_find_in_dir(dir_cluster, name83, 0xffffu, &ent, &dir_index) != 0) {
		mb_log_printf("fat_fopen: find fail dev=%u dir=%u path='%s'\n",
			      (uint32_t)dev, dir_cluster, mb_guarded_str(path));
		return MB_ERR_FILNF;
	}
	if (ent.attr & MB_FAT_ATTR_DIR) {
		mb_log_printf("fat_fopen: target is dir path='%s'\n",
			      mb_guarded_str(path));
		return MB_ERR_FILNF;
	}

	if (mb_fat_find_free_open_slot(&idx) != 0)
		return MB_ERR_NHNDL;

	mb_fat_open[idx].in_use = 1;
	mb_fat_open[idx].dev = mb_fat_vol->dev;
	mb_fat_open[idx].write = 0;
	mb_fat_open[idx].dirty = 0;
	mb_fat_open[idx].size = ent.size;
	mb_fat_open[idx].start_cluster = ent.start_lo;
	mb_fat_open[idx].offset = 0;
	mb_fat_open[idx].dir_cluster = dir_cluster;
	mb_fat_open[idx].dir_index = dir_index;
	mb_fat_open[idx].time = ent.wr_time;
	mb_fat_open[idx].date = ent.wr_date;
	mb_fat_open[idx].attr = ent.attr;
	return (long)(idx + 3);
}

long mb_fat_fcreate(const char *path, uint16_t mode)
{
	uint16_t dev;
	uint32_t dir_cluster;
	uint8_t name83[11];
	struct mb_fat_dirent ent;
	uint32_t dir_index = 0;
	uint32_t open_idx;
	uint8_t raw[32];
	int found;
	int rc;
	uint8_t attr;

	rc = mb_fat_locate_parent(path, &dev, &dir_cluster, name83);
	if (rc != 0)
		return rc;
	if (mb_fat_mount(dev) != 0)
		return MB_ERR_ACCDN;
	if (mb_fat_find_free_open_slot(&open_idx) != 0)
		return MB_ERR_NHNDL;

	found = (mb_fat_find_in_dir(dir_cluster, name83, 0xffffu, &ent, &dir_index) == 0);
	if (found) {
		if (ent.attr & MB_FAT_ATTR_DIR)
			return MB_ERR_ACCDN;
		if (ent.attr & MB_FAT_ATTR_RDONLY)
			return MB_ERR_ACCDN;
	} else {
		if (mb_fat_find_free_entry(dir_cluster, &dir_index) != 0)
			return MB_ERR_ACCDN;
	}

	if (found && ent.start_lo >= 2u) {
		if (mb_fat_free_chain(ent.start_lo) != 0)
			return MB_ERR_ACCDN;
	}

	attr = (uint8_t)(mode & (MB_FAT_ATTR_RDONLY |
				 MB_FAT_ATTR_HIDDEN |
				 MB_FAT_ATTR_SYSTEM));
	attr |= MB_FAT_ATTR_ARCH;

	memset(raw, 0, sizeof(raw));
	memcpy(raw, name83, sizeof(name83));
	raw[11] = attr;
	if (mb_fat_write_dirent_raw(dir_cluster, dir_index, raw) != 0)
		return MB_ERR_ACCDN;

	mb_fat_open[open_idx].in_use = 1;
	mb_fat_open[open_idx].dev = mb_fat_vol->dev;
	mb_fat_open[open_idx].write = 1;
	mb_fat_open[open_idx].dirty = 0;
	mb_fat_open[open_idx].size = 0;
	mb_fat_open[open_idx].start_cluster = 0;
	mb_fat_open[open_idx].offset = 0;
	mb_fat_open[open_idx].dir_cluster = dir_cluster;
	mb_fat_open[open_idx].dir_index = dir_index;
	mb_fat_open[open_idx].time = 0;
	mb_fat_open[open_idx].date = 0;
	mb_fat_open[open_idx].attr = attr;
	return (long)(open_idx + 3);
}

long mb_fat_fwrite(uint16_t handle, uint32_t cnt, void *buf)
{
	struct mb_fat_open *op;
	uint8_t sector[512];
	uint32_t remaining = cnt;
	uint8_t *src = (uint8_t *)buf;
	uint32_t written = 0;
	uint32_t cluster_size;

	op = mb_fat_get_open(handle);
	if (!op)
		return MB_ERR_IHNDL;
	if (!op->write)
		return MB_ERR_ACCDN;
	if (mb_fat_mount(op->dev) != 0)
		return MB_ERR_ACCDN;
	if (cnt == 0)
		return 0;

	cluster_size = mb_fat_vol->spc * mb_fat_vol->recsiz;
	if (cluster_size == 0)
		return MB_ERR_ACCDN;

	while (remaining > 0) {
		uint32_t cluster_index = op->offset / cluster_size;
		uint32_t cluster_off = op->offset % cluster_size;
		uint32_t cluster;
		uint32_t sector_in_cluster;
		uint32_t sector_off;
		uint32_t sector_idx;
		uint32_t chunk;

		if (mb_fat_ensure_cluster(op, cluster_index, &cluster) != 0)
			break;

		sector_in_cluster = cluster_off / mb_fat_vol->recsiz;
		sector_off = cluster_off % mb_fat_vol->recsiz;
		sector_idx = mb_fat_cluster_sector(cluster) + sector_in_cluster;
		chunk = mb_fat_vol->recsiz - sector_off;
		if (chunk > remaining)
			chunk = remaining;

		if (chunk != mb_fat_vol->recsiz) {
			if (mb_fat_rwabs(0, sector, 1, sector_idx, mb_fat_vol->dev) != 0)
				break;
		}
		memcpy(&sector[sector_off], src, chunk);
		if (mb_fat_rwabs(1, sector, 1, sector_idx, mb_fat_vol->dev) != 0)
			break;

		src += chunk;
		written += chunk;
		remaining -= chunk;
		op->offset += chunk;
		if (op->offset > op->size)
			op->size = op->offset;
		op->dirty = 1;
	}

	return (long)written;
}

long mb_fat_fdelete(const char *path)
{
	uint16_t dev;
	uint32_t dir_cluster;
	uint8_t name83[11];
	struct mb_fat_dirent ent;
	uint32_t dir_index = 0;
	uint8_t raw[32];
	int rc;

	rc = mb_fat_locate_parent(path, &dev, &dir_cluster, name83);
	if (rc != 0)
		return (rc == MB_ERR_DRIVE) ? MB_ERR_DRIVE : MB_ERR_FILNF;
	if (mb_fat_mount(dev) != 0)
		return MB_ERR_FILNF;
	if (mb_fat_find_in_dir(dir_cluster, name83, 0xffffu, &ent, &dir_index) != 0)
		return MB_ERR_FILNF;
	if (ent.attr & MB_FAT_ATTR_DIR)
		return MB_ERR_ACCDN;
	if (ent.attr & MB_FAT_ATTR_RDONLY)
		return MB_ERR_ACCDN;

	if (mb_fat_read_dirent_raw(dir_cluster, dir_index, raw) != 0)
		return MB_ERR_FILNF;
	raw[0] = 0xe5;
	if (mb_fat_write_dirent_raw(dir_cluster, dir_index, raw) != 0)
		return MB_ERR_ACCDN;
	if (ent.start_lo >= 2u && mb_fat_free_chain(ent.start_lo) != 0)
		return MB_ERR_ACCDN;
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

	if (op->dirty) {
		if (mb_fat_sync_open_dirent(op) != 0)
			return MB_ERR_ACCDN;
	}

	op->in_use = 0;
	return 0;
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
