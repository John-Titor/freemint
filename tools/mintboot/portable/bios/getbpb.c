#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"

#include <stdint.h>

struct mb_rom_bpb {
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

static uint16_t mb_rom_le16(const uint8_t *ptr)
{
	return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

static uint32_t mb_rom_le32(const uint8_t *ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
	       ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

long mb_bios_getbpb(uint16_t dev)
{
	uint8_t sector[512];
	uint32_t part_lba;
	uint16_t bytes_per_sec;
	uint8_t sec_per_clus;
	uint16_t rsvd_secs;
	uint8_t num_fats;
	uint16_t root_entries;
	uint16_t total_secs16;
	uint32_t total_secs32;
	uint16_t fatsz;
	uint16_t rdlen;
	uint32_t total_secs;
	uint32_t data_start;
	uint32_t numcl;
	struct mb_rom_bpb *bpb;
	static struct mb_rom_bpb mb_bpb;

	if (dev > 25)
		return MB_ERR_DRIVE;

	if (mb_rom_dispatch.rwabs(0, sector, 1, 0, dev) != 0)
		return 0;

	if (sector[510] != 0x55 || sector[511] != 0xaa)
		return 0;

	part_lba = mb_rom_le32(&sector[0x1be + 8]);
	if (part_lba == 0)
		return 0;

	if (mb_rom_dispatch.rwabs(0, sector, 1, (uint16_t)part_lba, dev) != 0)
		return 0;

	bytes_per_sec = mb_rom_le16(&sector[0x0b]);
	sec_per_clus = sector[0x0d];
	rsvd_secs = mb_rom_le16(&sector[0x0e]);
	num_fats = sector[0x10];
	root_entries = mb_rom_le16(&sector[0x11]);
	total_secs16 = mb_rom_le16(&sector[0x13]);
	fatsz = mb_rom_le16(&sector[0x16]);
	total_secs32 = mb_rom_le32(&sector[0x20]);

	if (bytes_per_sec == 0 || sec_per_clus == 0 || fatsz == 0)
		return 0;

	total_secs = total_secs16 ? total_secs16 : total_secs32;
	if (total_secs == 0)
		return 0;

	rdlen = (uint16_t)((root_entries * 32u + (bytes_per_sec - 1u)) /
			   bytes_per_sec);
	data_start = rsvd_secs + (uint32_t)num_fats * fatsz + rdlen;
	if (data_start >= total_secs)
		return 0;

	numcl = (total_secs - data_start) / sec_per_clus;

	bpb = &mb_bpb;
	bpb->recsiz = bytes_per_sec;
	bpb->clsiz = (int16_t)sec_per_clus;
	bpb->clsizb = bytes_per_sec * sec_per_clus;
	bpb->rdlen = (int16_t)rdlen;
	bpb->fsiz = (int16_t)fatsz;
	bpb->fatrec = (int16_t)(part_lba + rsvd_secs + fatsz);
	bpb->datrec = (int16_t)(part_lba + data_start);
	bpb->numcl = (uint16_t)numcl;
	bpb->bflags = 0;
	if (num_fats == 1)
		bpb->bflags |= (1 << 1);
	bpb->bflags |= 1;

	return (long)(uintptr_t)bpb;
}
