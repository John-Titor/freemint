#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_trap_helpers.h"
#include <stddef.h>
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

static void mb_kbdvec_stub(void)
{
}

struct mb_iorec {
	uint8_t *buf;
	uint16_t size;
	volatile uint16_t head;
	volatile uint16_t tail;
	uint16_t low;
	uint16_t high;
} __attribute__((packed));

struct mb_ext_iorec {
	struct mb_iorec in;
	struct mb_iorec out;
	uint8_t baudrate;
	uint8_t flowctrl;
	uint8_t ucr;
	uint8_t datamask;
	uint8_t wr5;
} __attribute__((packed));

struct mb_maptab {
	long (*bconstat)(void);
	long (*bconin)(void);
	long (*bcostat)(void);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rsconf)(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs,
		       uint16_t ts, uint16_t sc);
	struct mb_ext_iorec *iorec;
} __attribute__((packed));

struct mb_bconmap {
	struct mb_maptab *maptab;
	uint16_t maptabsize;
	uint16_t mapped_device;
} __attribute__((packed));

static long mb_bconmap_bconstat(void)
{
	return mb_rom_dispatch.bconstat(6);
}

static long mb_bconmap_bconin(void)
{
	return mb_rom_dispatch.bconin(6);
}

static long mb_bconmap_bcostat(void)
{
	return mb_rom_dispatch.bcostat(6);
}

static long mb_bconmap_bconout(uint16_t dev, uint16_t c)
{
	(void)dev;
	return mb_rom_dispatch.bconout(6, c);
}

static long mb_bconmap_rsconf(uint16_t baud, uint16_t flow, uint16_t uc,
			      uint16_t rs, uint16_t ts, uint16_t sc)
{
	return mb_rom_rsconf(baud, flow, uc, rs, ts, sc);
}

static struct mb_ext_iorec *mb_bconmap_iorec(void)
{
	enum { mb_rs232_bufsize = 256 };
	static uint8_t mb_iorec_ibuf[mb_rs232_bufsize];
	static uint8_t mb_iorec_obuf[mb_rs232_bufsize];
	static struct mb_ext_iorec mb_iorec;
	static int inited;

	if (!inited) {
		mb_iorec.in.buf = mb_iorec_ibuf;
		mb_iorec.in.size = mb_rs232_bufsize;
		mb_iorec.in.head = 0;
		mb_iorec.in.tail = 0;
		mb_iorec.in.low = mb_rs232_bufsize / 4;
		mb_iorec.in.high = (mb_rs232_bufsize * 3) / 4;
		mb_iorec.out.buf = mb_iorec_obuf;
		mb_iorec.out.size = mb_rs232_bufsize;
		mb_iorec.out.head = 0;
		mb_iorec.out.tail = 0;
		mb_iorec.out.low = mb_rs232_bufsize / 4;
		mb_iorec.out.high = (mb_rs232_bufsize * 3) / 4;
		mb_iorec.baudrate = 0;
		mb_iorec.flowctrl = 0;
		mb_iorec.ucr = 0;
		mb_iorec.datamask = 0;
		mb_iorec.wr5 = 0;
		inited = 1;
	}

	return &mb_iorec;
}

static struct mb_bconmap *mb_bconmap_root(void)
{
	static struct mb_maptab map = {
		.bconstat = mb_bconmap_bconstat,
		.bconin = mb_bconmap_bconin,
		.bcostat = mb_bconmap_bcostat,
		.bconout = mb_bconmap_bconout,
		.rsconf = mb_bconmap_rsconf,
		.iorec = NULL,
	};
	static struct mb_bconmap root = {
		.maptab = &map,
		.maptabsize = 1,
		.mapped_device = 6,
	};

	if (!map.iorec)
		map.iorec = mb_bconmap_iorec();

	return &root;
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
	if (dev > 25)
		return MB_ERR_DRIVE;

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
	uint32_t base;

	if (vnum >= 256)
		return 0;

	base = mb_portable_vector_base();
	vectors = (volatile uint32_t *)(uintptr_t)base;
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

long mb_rom_bcostat(uint16_t dev)
{
	if (dev == 2)
		return -1;
	return 0;
}

long mb_rom_drvmap(void)
{
	return 0;
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
	if (io_dev == 0)
		return (long)(uintptr_t)mb_bconmap_iorec();
	return -1;
}

long mb_rom_rsconf(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs, uint16_t ts, uint16_t sc)
{
	struct mb_ext_iorec *iorec = mb_bconmap_iorec();

	if (baud != 0xffffu)
		iorec->baudrate = (uint8_t)baud;
	if (flow != 0xffffu)
		iorec->flowctrl = (uint8_t)flow;
	if (uc != 0xffffu)
		iorec->ucr = (uint8_t)uc;
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
	struct mb_keytab {
		uint8_t *nrml;
		uint8_t *shft;
		uint8_t *caps;
	} __attribute__((packed));
	static uint8_t keytab_data[128];
	static struct mb_keytab keytab = {
		.nrml = keytab_data,
		.shft = keytab_data,
		.caps = keytab_data,
	};

	(void)nrml;
	(void)shft;
	(void)caps;

	return (long)(uintptr_t)&keytab;
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
	struct mb_kbdvecs {
		uint32_t midivec;
		uint32_t vkbderr;
		uint32_t vmiderr;
		uint32_t statvec;
		uint32_t mousevec;
		uint32_t clockvec;
		uint32_t joyvec;
		uint32_t midisys;
		uint32_t ikbdsys;
		uint8_t ikbdstate;
		uint8_t kbdlength;
		uint16_t pad;
	} __attribute__((packed));
	struct mb_kbdvbase {
		uint32_t kbdvec;
		struct mb_kbdvecs vecs;
	} __attribute__((packed));
	static struct mb_kbdvbase storage;
	static int inited;

	if (!inited) {
		uint32_t stub = (uint32_t)(uintptr_t)mb_kbdvec_stub;
		storage.kbdvec = stub;
		storage.vecs.midivec = stub;
		storage.vecs.vkbderr = stub;
		storage.vecs.vmiderr = stub;
		storage.vecs.statvec = stub;
		storage.vecs.mousevec = stub;
		storage.vecs.clockvec = stub;
		storage.vecs.joyvec = stub;
		storage.vecs.midisys = stub;
		storage.vecs.ikbdsys = stub;
		storage.vecs.ikbdstate = 0;
		storage.vecs.kbdlength = 0;
		inited = 1;
	}

	return (long)(uintptr_t)&storage.vecs;
}

long mb_rom_vsync(void)
{
	return 0;
}

long mb_rom_bconmap(uint16_t dev)
{
	struct mb_bconmap *root = mb_bconmap_root();
	uint16_t old_dev = root->mapped_device;

	if (dev == 0xffffu)
		return old_dev;

	if (dev == 0xfffeu)
		return (long)(uintptr_t)root;

	if (dev != 6)
		return 0;

	root->mapped_device = 6;
	return old_dev;
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

long mb_rom_bios_dispatch(uint16_t fnum, uint16_t *args)
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
					     (void *)(uintptr_t)mb_arg32w(args, 1),
					     mb_arg16(args, 3), mb_arg16(args, 4),
					     mb_arg16(args, 5));
	case 0x05:
		return mb_rom_setexc(mb_arg16(args, 0), mb_arg32w(args, 1));
	case 0x07:
		return mb_rom_getbpb(mb_arg16(args, 0));
	case 0x08:
		return mb_rom_dispatch.bcostat(mb_arg16(args, 0));
	case 0x0a:
		return mb_rom_dispatch.drvmap();
	case 0x0b:
		return mb_rom_kbshift(mb_arg16(args, 0));
	default:
		mb_log_printf("bios: unhandled 0x%04x", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}

long mb_rom_xbios_dispatch(uint16_t fnum, uint16_t *args)
{
	switch (fnum) {
	case 0x00:
		return mb_rom_initmous(mb_arg16(args, 0), mb_arg32w(args, 1), mb_arg32w(args, 3));
	case 0x04:
		return mb_rom_getrez();
	case 0x0e:
		return mb_rom_iorec(mb_arg16(args, 0));
	case 0x0f:
		return mb_rom_rsconf(mb_arg16(args, 0), mb_arg16(args, 1), mb_arg16(args, 2), mb_arg16(args, 3), mb_arg16(args, 4), mb_arg16(args, 5));
	case 0x10:
		return mb_rom_keytbl(mb_arg32w(args, 0), mb_arg32w(args, 2), mb_arg32w(args, 4));
	case 0x15:
		return mb_rom_cursconf(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x16:
		return mb_rom_settime(mb_arg32w(args, 0));
	case 0x17:
		return mb_rom_gettime();
	case 0x18:
		return mb_rom_bioskeys();
	case 0x1d:
		return mb_rom_offgibit(mb_arg16(args, 0));
	case 0x1e:
		return mb_rom_ongibit(mb_arg16(args, 0));
	case 0x20:
		return mb_rom_dosound(mb_arg32w(args, 0));
	case 0x22:
		return mb_rom_kbdvbase();
	case 0x23:
		return mb_rom_kbrate(mb_arg16(args, 0), mb_arg16(args, 1));
	case 0x25:
		return mb_rom_vsync();
	case 0x2c:
		return mb_rom_bconmap(mb_arg16(args, 0));
	case 0x05:
		return mb_rom_vsetscreen(mb_arg32w(args, 0), mb_arg32w(args, 2), mb_arg16(args, 4), mb_arg16(args, 5));
	default:
		mb_log_printf("xbios: unhandled 0x%04x", (uint32_t)fnum);
		return MB_ERR_INVFN;
	}
}
