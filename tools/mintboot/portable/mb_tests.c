#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_tests.h"
#include "mintboot/mb_lowmem.h"

#ifndef str
#define str(x) _stringify(x)
#define _stringify(x) #x
#endif
#include "../../../sys/buildinfo/version.h"

static uint32_t mb_tests_strlen(const char *s)
{
	uint32_t len = 0;

	while (s[len])
		len++;
	return len;
}

static void mb_tests_strcat(char *dst, uint32_t dstsz, const char *src)
{
	uint32_t dlen = mb_tests_strlen(dst);
	uint32_t i = 0;

	if (dlen >= dstsz)
		mb_panic("Test strcat overflow");

	while (src[i] && dlen + i + 1 < dstsz) {
		dst[dlen + i] = src[i];
		i++;
	}
	dst[dlen + i] = '\0';

	if (src[i])
		mb_panic("Test strcat overflow");
}

struct mb_test_iorec {
	uint8_t *buf;
	uint16_t size;
	volatile uint16_t head;
	volatile uint16_t tail;
	uint16_t low;
	uint16_t high;
} __attribute__((packed));

struct mb_test_ext_iorec {
	struct mb_test_iorec in;
	struct mb_test_iorec out;
	uint8_t baudrate;
	uint8_t flowctrl;
	uint8_t ucr;
	uint8_t datamask;
	uint8_t wr5;
} __attribute__((packed));

struct mb_test_maptab {
	long (*bconstat)(void);
	long (*bconin)(void);
	long (*bcostat)(void);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rsconf)(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs,
		       uint16_t ts, uint16_t sc);
	struct mb_test_ext_iorec *iorec;
} __attribute__((packed));

struct mb_test_bconmap {
	struct mb_test_maptab *maptab;
	uint16_t maptabsize;
	uint16_t mapped_device;
} __attribute__((packed));

static void mb_setexc_tests(void)
{
	const uint16_t vnum = 200;
	const uint32_t new_vec = 0x12345678u;
	const uint32_t alt_vec = 0x87654321u;
	long prev;
	long cur;
	long rc;

	prev = (uint32_t)(uintptr_t)Setexc(vnum, 0xffffffffu);
	cur = (uint32_t)(uintptr_t)Setexc(vnum, new_vec);
	if (cur != prev)
		mb_panic("Setexc: prev mismatch %d vs %d", (int)cur, (int)prev);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, 0xffffffffu);
	if ((uint32_t)cur != new_vec)
		mb_panic("Setexc: query mismatch %d vs %u", (int)cur, new_vec);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, alt_vec);
	if ((uint32_t)cur != new_vec)
		mb_panic("Setexc: swap mismatch %d vs %u", (int)cur, new_vec);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, prev);
	if ((uint32_t)cur != alt_vec)
		mb_panic("Setexc: restore mismatch %d vs %u", (int)cur, alt_vec);

	rc = (long)(uintptr_t)Setexc(256, 0xffffffffu);
	if (rc != 0)
		mb_panic("Setexc: vnum range rc=%d expected 0", (int)rc);
}

static void mb_gettime_tests(void)
{
	uint32_t dt = (uint32_t)Gettime();
	uint16_t date = (uint16_t)(dt >> 16);
	uint16_t time = (uint16_t)dt;
	uint16_t year = (uint16_t)(1980u + ((date >> 9) & 0x7fu));
	uint16_t month = (uint16_t)((date >> 5) & 0x0fu);
	uint16_t day = (uint16_t)(date & 0x1fu);
	uint16_t hour = (uint16_t)((time >> 11) & 0x1fu);
	uint16_t minute = (uint16_t)((time >> 5) & 0x3fu);
	uint16_t second = (uint16_t)((time & 0x1fu) * 2u);
	uint32_t packed_min;
	uint32_t packed_got;

	if (month < 1 || month > 12)
		mb_panic("Gettime: month=%u", (uint32_t)month);
	if (day < 1 || day > 31)
		mb_panic("Gettime: day=%u", (uint32_t)day);
	if (hour > 23)
		mb_panic("Gettime: hour=%u", (uint32_t)hour);
	if (minute > 59)
		mb_panic("Gettime: minute=%u", (uint32_t)minute);
	if (second > 59)
		mb_panic("Gettime: second=%u", (uint32_t)second);

	packed_min = (uint32_t)(((2026u - 1980u) << 9) | (1u << 5) | 1u);
	packed_got = (uint32_t)(((year - 1980u) << 9) | (month << 5) | day);
	if (packed_got < packed_min)
		mb_panic("Gettime: date=%u/%u/%u below 2026-01-01",
			 (uint32_t)year, (uint32_t)month, (uint32_t)day);
}

static void mb_drive_range_tests(void)
{
	uint8_t sector[512];
	long rc;

	rc = Rwabs(0, sector, 1, 0, 26);
	if (rc != MB_ERR_DRIVE)
	mb_panic("Rwabs drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = (long)(uintptr_t)Getbpb(26);
	if (rc != MB_ERR_DRIVE)
	mb_panic("Getbpb drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);

	rc = Dfree(sector, 26);
	if (rc != MB_ERR_DRIVE)
	mb_panic("Dfree drive rc=%d expected %d", (int)rc, MB_ERR_DRIVE);
}

static void mb_bconmap_tests(void)
{
	struct mb_test_bconmap *root;
	struct mb_test_ext_iorec *iorec;
	long rc;

	rc = Bconmap(0xffffu);
	if (rc != 6)
		mb_panic("Bconmap current rc=%d expected 6", (int)rc);

	rc = Bconmap(0xfffeu);
	if (rc == 0)
		mb_panic("Bconmap pointer rc=0");

	root = (struct mb_test_bconmap *)(uintptr_t)rc;
	if (root->maptabsize != 1)
		mb_panic("Bconmap maptabsize=%u", (uint32_t)root->maptabsize);
	if (root->mapped_device != 6)
		mb_panic("Bconmap mapped_device=%u", (uint32_t)root->mapped_device);

	if (!root->maptab)
		mb_panic("Bconmap maptab null");
	if (!root->maptab[0].bconstat || !root->maptab[0].bconin ||
	    !root->maptab[0].bcostat || !root->maptab[0].bconout ||
	    !root->maptab[0].rsconf || !root->maptab[0].iorec)
		mb_panic("Bconmap maptab entry missing");

	rc = Bconmap(6);
	if (rc != 6)
		mb_panic("Bconmap set rc=%d expected 6", (int)rc);

	rc = Bconmap(5);
	if (rc != 0)
		mb_panic("Bconmap invalid rc=%d expected 0", (int)rc);

	rc = (long)(uintptr_t)Iorec(0);
	if (rc == -1)
		mb_panic("Iorec(0) returned -1");
	iorec = (struct mb_test_ext_iorec *)(uintptr_t)rc;

	if (!iorec->in.buf || !iorec->out.buf)
		mb_panic("Iorec buffers not set");

	if ((uintptr_t)root->maptab[0].iorec != (uintptr_t)iorec)
		mb_panic("Iorec pointer mismatch");

	rc = (long)(uintptr_t)Iorec(1);
	if (rc != -1)
		mb_panic("Iorec(1) rc=%d expected -1", (int)rc);

	rc = Rsconf(0x1234u, 0x55u, 0x66u, 0xffffu, 0xffffu, 0xffffu);
	if (rc != 0)
		mb_panic("Rsconf rc=%d expected 0", (int)rc);

	if (iorec->baudrate != 0x34u || iorec->flowctrl != 0x55u ||
	    iorec->ucr != 0x66u)
		mb_panic("Rsconf did not update iorec");

	rc = Rsconf(0xffffu, 0xffffu, 0xffffu, 0xffffu, 0xffffu, 0xffffu);
	if (rc != 0)
		mb_panic("Rsconf -1 rc=%d expected 0", (int)rc);
	if (iorec->baudrate != 0x34u || iorec->flowctrl != 0x55u ||
	    iorec->ucr != 0x66u)
		mb_panic("Rsconf -1 updated iorec");
}

static void mb_vbclock_tests(void)
{
	volatile uint32_t *vbclock = mb_lm_vbclock();
	void (*handler)(void) = (void (*)(void))(uintptr_t)*mb_lm_etv_timer();
	uint32_t start;
	uint32_t current;
	uint32_t spins;

	start = *vbclock;
	for (spins = 0; spins < 5000000u; spins++) {
		if (handler)
			handler();
		current = *vbclock;
		if (current != start)
			return;
	}

	mb_panic("VBCLOCK did not increment");
}

static void mb_drive_path_tests(void)
{
	char buf[256];
	long rc;
	uint16_t dev;
	uint32_t map;
	uint16_t invalid_drive = 0xffffu;
	const char *long_rel;
	char longbuf[300];
	uint32_t i;

	dev = mb_portable_boot_drive();
	if (dev >= 26)
		mb_panic("Drive tests: invalid boot drive");
	map = (uint32_t)mb_rom_dispatch.drvmap();
	for (i = 0; i < 26; i++) {
		if ((map & (1u << i)) == 0) {
			invalid_drive = (uint16_t)i;
			break;
		}
	}

	rc = Dgetdrv();
	if (rc != (long)dev)
		mb_panic("Dgetdrv: initial=%d expected %d", (int)rc, (int)dev);

	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath(0) rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath(0) missing root");

	if (invalid_drive != 0xffffu) {
		rc = Dsetdrv(invalid_drive);
		if (rc != (long)map)
			mb_panic("Dsetdrv invalid rc=%d", (int)rc);
		if (Dgetdrv() != (long)dev)
			mb_panic("Dsetdrv invalid changed drive");
	}

	rc = Dsetdrv(dev);
	if (rc != (long)mb_rom_dispatch.drvmap())
		mb_panic("Dsetdrv current rc=%d", (int)rc);
	if (Dgetdrv() != (long)dev)
		mb_panic("Dsetdrv current mismatch");

	rc = Dsetpath("C:");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);
	rc = Dsetpath("C:FOO");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);
	rc = Dsetpath("C:\\MINT");
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath drive letter rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Dsetpath("no_such_dir");
	if (rc == 0)
		mb_panic("Dsetpath missing dir accepted");

	rc = Dsetpath("\\");
	if (rc != 0)
		mb_panic("Dsetpath root rc=%d", (int)rc);

	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath(0) after root rc=%d", (int)rc);
	if (buf[0] != '\\' || buf[1] != '\0')
		mb_panic("Dgetpath root mismatch");

	rc = Dsetpath("MINT");
	if (rc != 0)
		mb_panic("Dsetpath rel rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath after rel rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath rel missing root");

	rc = Dsetpath("\\MINT");
	if (rc != 0)
		mb_panic("Dsetpath abs rc=%d", (int)rc);

	rc = Dsetpath("1-19-cur");
	if (rc != 0)
		mb_panic("Dsetpath rel2 rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath rel2 rc=%d", (int)rc);
	if (buf[0] != '\\')
		mb_panic("Dgetpath rel2 missing root");

	long_rel = "\\MINT\\1-19-cur";
	rc = Dsetpath(long_rel);
	if (rc != 0)
		mb_panic("Dsetpath long rc=%d", (int)rc);
	rc = Dgetpath(buf, 0);
	if (rc != 0)
		mb_panic("Dgetpath long rc=%d", (int)rc);

	for (i = 0; i < sizeof(longbuf) - 1; i++)
		longbuf[i] = 'A';
	longbuf[sizeof(longbuf) - 1] = '\0';
	rc = Dsetpath(longbuf);
	if (rc != MB_ERR_PTHNF)
		mb_panic("Dsetpath long rc=%d expected %d", (int)rc, MB_ERR_PTHNF);

	rc = Dgetpath(buf, (uint16_t)(dev + 1u));
	if (rc != 0)
		mb_panic("Dgetpath current rc=%d", (int)rc);

	rc = Dgetpath(buf, 27);
	if (rc != MB_ERR_DRIVE)
		mb_panic("Dgetpath bad drive rc=%d", (int)rc);
}

static void mb_kernel_loader_tests(void)
{
	struct mb_prg_header {
		uint16_t magic;
		uint32_t tlen;
		uint32_t dlen;
		uint32_t blen;
		uint32_t slen;
		uint32_t res1;
		uint32_t flags;
		uint16_t abs;
	} __attribute__((packed));
	struct mb_aout_header {
		uint32_t magic;
		uint32_t text;
		uint32_t data;
		uint32_t bss;
		uint32_t syms;
		uint32_t entry;
		uint32_t trsize;
		uint32_t drsize;
	} __attribute__((packed));
	struct mb_basepage {
		uint8_t *p_lowtpa;
		uint8_t *p_hitpa;
		uint8_t *p_tbase;
		uint32_t p_tlen;
		uint8_t *p_dbase;
		uint32_t p_dlen;
		uint8_t *p_bbase;
		uint32_t p_blen;
		void *p_xdta;
		struct mb_basepage *p_parent;
		uint32_t p_flags;
		char *p_env;
		uint8_t p_uft[6];
		char p_lddrv;
		uint8_t p_curdrv;
		uint32_t p_1fill[2];
		uint8_t p_curdir[32];
		uint32_t p_3fill[2];
		uint32_t p_dreg;
		uint32_t p_areg[5];
		char p_cmdlin[0x80];
	} __attribute__((packed));
	char kernel_path[96];
	struct mb_prg_header hdr;
	struct mb_aout_header aout;
	uint16_t handle;
	long rc;
	uint32_t bp_addr;
	struct mb_basepage *bp;
	char drive = 'C';
	uint16_t dev;
	uint32_t tlen;
	uint32_t dlen;
	uint32_t blen;
#define MB_MINT_EXT_MAGIC 0x4d694e54u

	mb_cmdline[0] = '\0';

	dev = mb_portable_boot_drive();
	if (dev >= 26)
		mb_panic("Kernel test: invalid boot drive");
	drive = (char)('A' + dev);
	kernel_path[0] = drive;
	kernel_path[1] = ':';
	kernel_path[2] = '\\';
	kernel_path[3] = '\0';
	mb_tests_strcat(kernel_path, sizeof(kernel_path), "MINT\\");
	mb_tests_strcat(kernel_path, sizeof(kernel_path), MINT_VERS_PATH_STRING);
	mb_tests_strcat(kernel_path, sizeof(kernel_path), "\\MINT040.PRG");

	handle = (uint16_t)Fopen(kernel_path, 0);
	if ((int16_t)handle < 0)
		mb_panic("Kernel test: open failed");
	if (Fread(handle, sizeof(hdr), &hdr) != (long)sizeof(hdr))
		mb_panic("Kernel test: header read");
	tlen = hdr.tlen;
	dlen = hdr.dlen;
	blen = hdr.blen;
	if (hdr.res1 == MB_MINT_EXT_MAGIC) {
		if (Fseek(0x24, handle, 0) < 0)
			mb_panic("Kernel test: header seek");
		if (Fread(handle, sizeof(aout), &aout) != (long)sizeof(aout))
			mb_panic("Kernel test: aout header read");
		if (aout.magic == 0x0107u || aout.magic == 0x0108u ||
		    aout.magic == 0x010bu) {
			if (aout.entry + aout.text != hdr.tlen)
				mb_panic("Kernel test: aout size mismatch");
		}
	}
	Fclose(handle);

	rc = mb_portable_load_kernel(kernel_path, 0);
	if (rc != 0)
		mb_panic("Kernel test: load rc=%d", (int)rc);

	bp_addr = mb_portable_last_basepage();
	if (bp_addr == 0)
		mb_panic("Kernel test: basepage missing");
	bp = (struct mb_basepage *)(uintptr_t)bp_addr;
	if (bp->p_tlen != tlen || bp->p_dlen != dlen || bp->p_blen != blen)
		mb_panic("Kernel test: len mismatch");
	if (bp->p_tbase != (uint8_t *)(bp + 1))
		mb_panic("Kernel test: tbase mismatch");
	if (bp->p_parent || bp->p_xdta)
		mb_panic("Kernel test: basepage parent/dta not null");

}

void mb_portable_run_tests(void)
{
	mb_fat_run_tests();
	mb_setexc_tests();
	mb_gettime_tests();
	mb_drive_range_tests();
	mb_drive_path_tests();
	mb_bconmap_tests();
	mb_kernel_loader_tests();
	mb_vbclock_tests();
}
