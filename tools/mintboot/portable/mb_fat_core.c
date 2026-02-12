#include "mb_fat_internal.h"
#include "mintboot/mb_rom.h"
#include <stddef.h>
#include <string.h>

struct mb_fat_volume *mb_fat_vol;
struct mb_fat_volume mb_fat_vols[MB_FAT_MAX_VOLS];
static int8_t mb_fat_dev_map[26];
static uint8_t mb_fat_vol_rr;
static uint8_t mb_fat_dev_map_init;
struct mb_fat_search mb_fat_search[MB_FAT_MAX_SEARCH];
struct mb_fat_open mb_fat_open[MB_FAT_MAX_OPEN];

uint16_t mb_fat_le16(const uint8_t *ptr)
{
	return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

uint32_t mb_fat_le32(const uint8_t *ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
	       ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

int mb_fat_rwabs(uint16_t rw, void *buf, uint16_t count,
		 uint32_t recno, uint16_t dev)
{
	if (recno > 0xffffu)
		return -1;
	return (int)mb_rom_dispatch.rwabs(rw, buf, count, (uint16_t)recno, dev);
}

int mb_fat_mount(uint16_t dev)
{
	const struct mb_fat_bpb *bpb;
	long bpb_ptr;
	uint16_t num_fats;
	uint32_t root_start;
	uint32_t fat_start;
	struct mb_fat_volume *vol;
	int slot = -1;
	int i;

	if (dev >= 26)
		return MB_ERR_DRIVE;

	if (!mb_fat_dev_map_init) {
		for (i = 0; i < (int)(sizeof(mb_fat_dev_map) /
				      sizeof(mb_fat_dev_map[0])); i++)
			mb_fat_dev_map[i] = -1;
		mb_fat_dev_map_init = 1;
	}

	if (mb_fat_dev_map[dev] >= 0) {
		vol = &mb_fat_vols[mb_fat_dev_map[dev]];
		if (vol->valid && vol->dev == dev) {
			mb_fat_vol = vol;
			return 0;
		}
	}

	bpb_ptr = mb_rom_getbpb(dev);
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

	for (i = 0; i < MB_FAT_MAX_VOLS; i++) {
		if (!mb_fat_vols[i].valid) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		slot = mb_fat_vol_rr++ % MB_FAT_MAX_VOLS;
		if (mb_fat_vols[slot].valid) {
			uint8_t old_dev = mb_fat_vols[slot].dev;
			if (old_dev < 26 && mb_fat_dev_map[old_dev] == slot)
				mb_fat_dev_map[old_dev] = -1;
		}
		mb_fat_vols[slot].valid = 0;
	}

	vol = &mb_fat_vols[slot];
	vol->valid = 1;
	vol->dev = (uint8_t)dev;
	vol->recsiz = bpb->recsiz;
	vol->spc = (uint16_t)bpb->clsiz;
	vol->num_fats = num_fats;
	vol->fat_sectors = (uint16_t)bpb->fsiz;
	vol->fat_start = fat_start;
	vol->root_start = root_start;
	vol->root_sectors = (uint32_t)bpb->rdlen;
	vol->data_start = (uint32_t)bpb->datrec;
	vol->total_clusters = (uint32_t)bpb->numcl;
	mb_fat_dev_map[dev] = (int8_t)slot;
	mb_fat_vol = vol;
	return 0;
}

uint32_t mb_fat_cluster_sector(uint32_t cluster)
{
	if (cluster < 2)
		return 0;
	return mb_fat_vol->data_start +
	       ((cluster - 2u) * mb_fat_vol->spc);
}

uint16_t mb_fat_read_fat(uint32_t cluster)
{
	uint8_t sector[512];
	uint32_t offset = cluster * 2u;
	uint32_t sector_idx = offset / mb_fat_vol->recsiz;
	uint32_t sector_off = offset % mb_fat_vol->recsiz;

	if (mb_fat_rwabs(0, sector, 1,
			 mb_fat_vol->fat_start + sector_idx,
			 mb_fat_vol->dev) != 0)
		return 0xffffu;

	return mb_fat_le16(&sector[sector_off]);
}

int mb_fat_write_fat(uint32_t cluster, uint16_t value)
{
	uint8_t sector[512];
	uint32_t offset = cluster * 2u;
	uint32_t sector_idx = offset / mb_fat_vol->recsiz;
	uint32_t sector_off = offset % mb_fat_vol->recsiz;
	uint16_t fat;

	if (mb_fat_vol->recsiz != 512)
		return -1;

	for (fat = 0; fat < mb_fat_vol->num_fats; fat++) {
		uint32_t base = mb_fat_vol->fat_start +
				(uint32_t)fat * mb_fat_vol->fat_sectors;
		uint32_t sector_num = base + sector_idx;
		if (mb_fat_rwabs(0, sector, 1, sector_num,
				 mb_fat_vol->dev) != 0)
			return -1;
		sector[sector_off] = (uint8_t)(value & 0xff);
		sector[sector_off + 1] = (uint8_t)(value >> 8);
		if (mb_fat_rwabs(1, sector, 1, sector_num,
				 mb_fat_vol->dev) != 0)
			return -1;
	}

	return 0;
}

uint32_t mb_fat_alloc_cluster(void)
{
	uint32_t cluster;

	for (cluster = 2; cluster < (uint32_t)mb_fat_vol->total_clusters + 2u;
	     cluster++) {
		if (mb_fat_read_fat(cluster) == 0) {
			if (mb_fat_write_fat(cluster, 0xffffu) != 0)
				return 0;
			return cluster;
		}
	}

	return 0;
}

int mb_fat_free_chain(uint32_t start_cluster)
{
	uint32_t cluster = start_cluster;

	while (cluster >= 2 && cluster < 0xfff8u) {
		uint16_t next = mb_fat_read_fat(cluster);
		if (mb_fat_write_fat(cluster, 0) != 0)
			return -1;
		if (next >= 0xfff8u)
			break;
		cluster = next;
	}

	return 0;
}

int mb_fat_zero_cluster(uint32_t cluster)
{
	uint8_t sector[512];
	uint32_t i;

	memset(sector, 0, sizeof(sector));
	for (i = 0; i < mb_fat_vol->spc; i++) {
		uint32_t sector_idx = mb_fat_cluster_sector(cluster) + i;
		if (mb_fat_rwabs(1, sector, 1, sector_idx,
				 mb_fat_vol->dev) != 0)
			return -1;
	}

	return 0;
}

void mb_fat_pattern_83(const char *pattern, uint8_t out[11])
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

int mb_fat_match_83(const uint8_t name[11], const uint8_t pat[11])
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

void mb_fat_name_from_dirent(const struct mb_fat_dirent *ent, char out[14])
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

int mb_fat_has_wildcards(const char *name)
{
	for (; *name && *name != '\\' && *name != '/'; name++) {
		if (*name == '*' || *name == '?')
			return 1;
	}
	return 0;
}

int mb_fat_dirent_location(uint32_t dir_cluster, uint32_t index,
			  uint32_t *sector_idx, uint32_t *sector_off)
{
	uint32_t entries_per_sector = mb_fat_vol->recsiz / 32u;
	uint32_t cluster = dir_cluster;
	uint32_t entry_idx = index;

	if (mb_fat_vol->recsiz != 512)
		return -1;

	if (dir_cluster == 0) {
		uint32_t total_entries = (mb_fat_vol->root_sectors *
					  entries_per_sector);
		if (index >= total_entries)
			return -1;
		*sector_idx = mb_fat_vol->root_start +
			      (index / entries_per_sector);
		*sector_off = (index % entries_per_sector) * 32u;
		return 0;
	}

	while (entry_idx >= (mb_fat_vol->spc * entries_per_sector)) {
		uint16_t next = mb_fat_read_fat(cluster);
		if (next >= 0xfff8u)
			return -1;
		cluster = next;
		entry_idx -= mb_fat_vol->spc * entries_per_sector;
	}

	*sector_idx = mb_fat_cluster_sector(cluster) +
		      (entry_idx / entries_per_sector);
	*sector_off = (entry_idx % entries_per_sector) * 32u;
	return 0;
}

int mb_fat_read_dirent_raw(uint32_t dir_cluster, uint32_t index,
			  uint8_t raw[32])
{
	uint8_t sector[512];
	uint32_t sector_idx;
	uint32_t sector_off;

	if (mb_fat_dirent_location(dir_cluster, index, &sector_idx,
				   &sector_off) != 0)
		return -1;
	if (mb_fat_rwabs(0, sector, 1, sector_idx,
			 mb_fat_vol->dev) != 0)
		return -1;
	memcpy(raw, &sector[sector_off], 32u);
	return 0;
}

void mb_fat_decode_dirent(const uint8_t raw[32], struct mb_fat_dirent *out)
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

int mb_fat_write_dirent_raw(uint32_t dir_cluster, uint32_t index,
			   const uint8_t raw[32])
{
	uint8_t sector[512];
	uint32_t sector_idx;
	uint32_t sector_off;

	if (mb_fat_dirent_location(dir_cluster, index, &sector_idx,
				   &sector_off) != 0)
		return -1;
	if (mb_fat_rwabs(0, sector, 1, sector_idx,
			 mb_fat_vol->dev) != 0)
		return -1;
	memcpy(&sector[sector_off], raw, 32u);
	if (mb_fat_rwabs(1, sector, 1, sector_idx,
			 mb_fat_vol->dev) != 0)
		return -1;
	return 0;
}

int mb_fat_read_dirent(uint32_t dir_cluster, uint32_t index,
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
			 mb_fat_vol->dev) != 0)
		return -1;
	memcpy(raw, &sector[sector_off], 32u);
	mb_fat_decode_dirent(raw, out);
	return 0;
}

int mb_fat_parse_drive(const char *path, uint16_t *dev_out)
{
	uint16_t dev = mb_rom_get_current_drive();

	if (!path)
		return MB_ERR_PTHNF;

	if (path[1] == ':') {
		char drive = path[0];
		if (drive >= 'a' && drive <= 'z')
			drive = (char)(drive - 'a' + 'A');
		if (drive < 'A' || drive > 'Z')
			return MB_ERR_DRIVE;
		dev = (uint16_t)(drive - 'A');
	}

	*dev_out = dev;
	return 0;
}
