#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
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

/* BIOS (trap #13) stubs */
long mb_rom_bconstat(uint16_t dev)
{
	mb_panic("Bconstat(dev=%u)", (uint32_t)dev);
	return -1;
}

long mb_rom_bconin(uint16_t dev)
{
	mb_panic("Bconin(dev=%u)", (uint32_t)dev);
	return -1;
}

long mb_rom_bconout(uint16_t dev, uint16_t c)
{
	mb_panic("Bconout(dev=%u, c=%u)", (uint32_t)dev, (uint32_t)c);
	return -1;
}

long mb_rom_rwabs(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno,
		  uint16_t dev)
{
	(void)rwflag;
	(void)buf;
	(void)count;
	(void)recno;
	(void)dev;
	return -1;
}

long mb_rom_setexc(uint16_t vnum, uint32_t vptr)
{
	volatile uint32_t *vectors;
	uint32_t prev;

	if (vnum >= 256)
		return 0;

	vectors = (volatile uint32_t *)0x0;
	prev = vectors[vnum];

	if (vptr != 0xffffffffu)
		vectors[vnum] = vptr;

	return (long)prev;
}

long mb_rom_getbpb(uint16_t dev)
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

long mb_rom_bcostat(uint16_t dev)
{
	if (dev == 2)
		return -1;
	return 0;
}

long mb_rom_drvmap(void)
{
	return 1u << 2;
}

long mb_rom_kbshift(uint16_t data)
{
	(void)data;
	return 0;
}

/* XBIOS (trap #14) stubs */
long mb_rom_initmous(uint16_t type, uint32_t param, uint32_t vptr)
{
	(void)type;
	(void)param;
	(void)vptr;
	return -1;
}

long mb_rom_getrez(void)
{
	return 2;
}

long mb_rom_iorec(uint16_t io_dev)
{
	mb_panic("Iorec(dev=%u)", (uint32_t)io_dev);
	return -1;
}

long mb_rom_rsconf(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs, uint16_t ts, uint16_t sc)
{
	(void)baud;
	(void)flow;
	(void)uc;
	(void)rs;
	(void)ts;
	(void)sc;
	return 0;
}
long mb_rom_keytbl(uint32_t nrml, uint32_t shft, uint32_t caps)
{
	mb_panic("Keytbl(nrml=%08x, shft=%08x, caps=%08x)", nrml, shft, caps);
	return -1;
}

long mb_rom_cursconf(uint16_t rate, uint16_t attr)
{
	(void)attr;
	if (rate == 5 || rate == 7)
		return 20;
	return 0;
}

long mb_rom_settime(uint32_t time)
{
	(void)time;
	return 0;
}

long mb_rom_gettime(void)
{
	const uint16_t year = 2026;
	const uint16_t month = 1;
	const uint16_t day = 1;
	uint16_t date;

	date = (uint16_t)(((year - 1980u) << 9) | (month << 5) | day);
	return (long)(((uint32_t)date << 16) | 0u);
}

long mb_rom_bioskeys(void)
{
	return 0;
}

long mb_rom_offgibit(uint16_t ormask)
{
	(void)ormask;
	return 0;
}

long mb_rom_ongibit(uint16_t andmask)
{
	(void)andmask;
	return 0;
}

long mb_rom_dosound(uint32_t ptr)
{
	(void)ptr;
	return 0;
}

long mb_rom_kbdvbase(void)
{
	mb_panic("Kbdvbase()");
	return -1;
}

long mb_rom_vsync(void)
{
	return 0;
}

long mb_rom_bconmap(uint16_t dev)
{
	mb_panic("Bconmap(dev=%u)", (uint32_t)dev);
	return -1;
}

long mb_rom_vsetscreen(uint32_t lscrn, uint32_t pscrn, uint16_t rez, uint16_t mode)
{
	(void)lscrn;
	(void)pscrn;
	(void)rez;
	(void)mode;
	return -1;
}
long mb_rom_kbrate(uint16_t delay, uint16_t rate)
{
	(void)delay;
	(void)rate;
	return 0;
}

long mb_rom_bios_dispatch(uint16_t fnum, uint32_t *args)
{
	switch (fnum) {
	case 0x01:
		return mb_rom_dispatch.bconstat(mb_arg16(args, 0));
	case 0x02:
		return mb_rom_dispatch.bconin(mb_arg16(args, 0));
	case 0x03:
		return mb_rom_dispatch.bconout(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x04:
		return mb_rom_dispatch.rwabs(mb_arg16(args, 0),
					     (void *)(uintptr_t)mb_arg32(args, 1),
					     mb_arg16(args, 2), mb_arg16(args, 3),
					     mb_arg16(args, 4));
	case 0x05:
		return mb_rom_dispatch.setexc(mb_arg16(args, 0), mb_arg32(args, 1));
	case 0x07:
		return mb_rom_dispatch.getbpb(mb_arg16(args, 0));
	case 0x08:
		return mb_rom_dispatch.bcostat(mb_arg16(args, 0));
	case 0x0a:
		return mb_rom_dispatch.drvmap();
	case 0x0b:
		return mb_rom_dispatch.kbshift(mb_arg16(args, 0));
	default:
		mb_panic("bios: unhandled 0x%04x", (uint32_t)fnum);
	}

	return -1;
}

long mb_rom_xbios_dispatch(uint16_t fnum, uint32_t *args)
{
	switch (fnum) {
	case 0x00:
		return mb_rom_dispatch.initmous(mb_arg16(args, 0), mb_arg32(args, 1), mb_arg32(args, 2));
	case 0x04:
		return mb_rom_dispatch.getrez();
	case 0x0e:
		return mb_rom_dispatch.iorec(mb_arg16(args, 0));
	case 0x0f:
		return mb_rom_dispatch.rsconf(mb_arg16(args, 0), mb_arg16(args, 1), mb_arg16(args, 2), mb_arg16(args, 3), mb_arg16(args, 4), mb_arg16(args, 5));
	case 0x10:
		return mb_rom_dispatch.keytbl(mb_arg32(args, 0), mb_arg32(args, 1), mb_arg32(args, 2));
	case 0x15:
		return mb_rom_dispatch.cursconf(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x16:
		return mb_rom_dispatch.settime(mb_arg32(args, 0));
	case 0x17:
		return mb_rom_dispatch.gettime();
	case 0x18:
		return mb_rom_dispatch.bioskeys();
	case 0x1d:
		return mb_rom_dispatch.offgibit(mb_arg16(args, 0));
	case 0x1e:
		return mb_rom_dispatch.ongibit(mb_arg16(args, 0));
	case 0x20:
		return mb_rom_dispatch.dosound(mb_arg32(args, 0));
	case 0x22:
		return mb_rom_dispatch.kbdvbase();
	case 0x23:
		return mb_rom_dispatch.kbrate(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x25:
		return mb_rom_dispatch.vsync();
	case 0x2c:
		return mb_rom_dispatch.bconmap(mb_arg16(args, 0));
	case 0x05:
		return mb_rom_dispatch.vsetscreen(mb_arg32(args, 0), mb_arg32(args, 1), mb_arg16(args, 2), mb_arg16(args, 3));
	default:
		mb_panic("xbios: unhandled 0x%04x", (uint32_t)fnum);
	}

	return -1;
}
