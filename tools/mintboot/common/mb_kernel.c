#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_cpu.h"
#include "mintboot/mb_cookie.h"

#include <stddef.h>
#include "mintboot/mb_lib.h"

#ifndef str
#define str(x) _stringify(x)
#define _stringify(x) #x
#endif
#include "../../../sys/buildinfo/version.h"

#define MB_PRG_MAGIC 0x601au
#define MB_PRG_HDR_SIZE 28u
#define MB_PRG_BASE_ADDR 0x00010000u
#define MB_PRG_STACK_USER    0x00010000u
#define MB_PRG_STACK_SUP     0x00010000u
#define MB_PRG_STACK_RESERVE (MB_PRG_STACK_USER + MB_PRG_STACK_SUP)
#define MB_MINT_EXT_MAGIC 0x4d694e54u
#define MB_MINT_AOUT_OFF 0x24u
#define MB_MINT_TEXT_OFF 0x100u

extern uint8_t _mb_image_end[] __attribute__((weak));
static struct mb_basepage *mb_kernel_last_basepage;
static uint32_t mb_kernel_base;
static uint32_t mb_kernel_end;

uint32_t mb_common_last_basepage(void)
{
	return (uint32_t)(uintptr_t)mb_kernel_last_basepage;
}

void mb_common_kernel_bounds(uint32_t *base, uint32_t *end)
{
	if (base)
		*base = mb_kernel_base;
	if (end)
		*end = mb_kernel_end;
}

static int mb_path_exists(const char *path)
{
	long fh;

	fh = Fopen(path, 0);
	if (fh < 0)
		return 0;
	Fclose((uint16_t)fh);
	return 1;
}

static int mb_find_boot_drive(char *drive_out)
{
	uint16_t boot_drive = mb_common_boot_drive();

	if (boot_drive >= 26)
		return -1;
	*drive_out = (char)('A' + boot_drive);
	return 0;
}

int mb_common_find_kernel_path(char *out, size_t outsz)
{
	const char *name = "mint000.prg";
	uint32_t cpu;
	char drive;
	char prefix[4];

	if (!out || outsz < 4)
		return -1;

	if (mb_find_boot_drive(&drive) != 0)
		return -1;
	prefix[0] = drive;
	prefix[1] = ':';
	prefix[2] = '\\';
	prefix[3] = '\0';

	strlcpy(out, prefix, outsz);
	strlcat(out, "MINT\\", outsz);
	strlcat(out, MINT_VERS_PATH_STRING, outsz);
	strlcat(out, "\\", outsz);
	strlcat(out, name, outsz);
	if (mb_path_exists(out))
		return 0;

	if (mb_cookie_get(&mb_cookie_jar, MB_COOKIE_ID('_', 'C', 'P', 'U'), &cpu) == 0) {
		switch (cpu) {
		case 20:
			name = "mint020.prg";
			break;
		case 30:
			name = "mint030.prg";
			break;
		case 40:
			name = "mint040.prg";
			break;
		case 60:
			name = "mint060.prg";
			break;
		default:
			break;
		}
	}

	strlcpy(out, prefix, outsz);
	strlcat(out, "MINT\\", outsz);
	strlcat(out, MINT_VERS_PATH_STRING, outsz);
	strlcat(out, "\\", outsz);
	strlcat(out, name, outsz);
	if (mb_path_exists(out))
		return 0;

	return -1;
}

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

#define MB_PDCLSIZE 0x80

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
	char p_cmdlin[MB_PDCLSIZE];
} __attribute__((packed));

struct mb_osheader {
	uint16_t os_entry;
	uint16_t os_version;
	void *reseth;
	struct mb_osheader *os_beg;
	void *os_end;
	uint32_t os_rsvl;
	void *os_magic;
	int32_t os_date;
	uint16_t os_conf;
	uint16_t os_dosdate;
	uint8_t **p_root;
	uint8_t **pkbshift;
	struct mb_basepage **p_run;
	uint32_t p_rsv2;
} __attribute__((packed));

static long mb_read_exact(uint16_t handle, void *buf, uint32_t len)
{
	uint32_t offset = 0;
	uint8_t *dst = (uint8_t *)buf;

	while (offset < len) {
		long rc = Fread(handle, len - offset, dst + offset);
		if (rc <= 0)
			return (rc == 0) ? -1 : rc;
		offset += (uint32_t)rc;
	}

	return 0;
}

static int mb_relocate_prg(uint16_t handle, uint32_t reloc_off,
			   uint8_t *tbase, uint32_t text_data_len)
{
	uint32_t relst = 0;
	uint32_t offset = 0;
	uint32_t relbase = (uint32_t)(uintptr_t)tbase;
	uint8_t buf[256];

	if (Fseek((int32_t)reloc_off, handle, 0) < 0) {
		mb_log_printf("mintboot: reloc seek failed off=%08x\n", reloc_off);
		return -1;
	}

	if (mb_read_exact(handle, &relst, sizeof(relst)) != 0) {
		mb_log_printf("mintboot: reloc read start failed\n");
		return -1;
	}

	if (relst == 0)
		return 0;

	mb_log_printf("mintboot: PRG relocation base=%08x\n",
		      (uint32_t)(uintptr_t)tbase);

	offset = relst;
	if (offset >= text_data_len)
		return -1;
	*(uint32_t *)(void *)(tbase + offset) += relbase;

	for (;;) {
		long rc = Fread(handle, sizeof(buf), buf);
		uint32_t i;

		if (rc < 0) {
			mb_log_printf("mintboot: reloc read failed rc=%ld\n", rc);
			return -1;
		}
		if (rc == 0) {
			mb_log_printf("mintboot: reloc read hit EOF\n");
			break;
		}

		for (i = 0; i < (uint32_t)rc; i++) {
			uint8_t b = buf[i];
			if (b == 0)
				return 0;
			if (b == 1) {
				/* DRI extension byte: advance by 254, no relocation here. */
				offset += 254;
				continue;
			} else {
				offset += b;
			}
			if (offset >= text_data_len) {
				mb_log_printf("mintboot: reloc offset=%08x exceeds len=%08x\n",
					      offset, text_data_len);
				return -1;
			}
			*(uint32_t *)(void *)(tbase + offset) += relbase;
		}
	}

	mb_log_printf("mintboot: reloc failed (unexpected end)\n");
	return -1;
}

int mb_common_load_kernel(const char *path, int do_jump)
{
	struct mb_prg_header hdr;
	struct mb_aout_header aout;
	struct mb_basepage *bp;
	uint16_t handle;
	uint32_t text_data_len;
	uint32_t total_len;
	uint32_t tpa_start;
	uint32_t tpa_end;
	uint32_t text_off;
	uint32_t reloc_off;
	uint32_t entry_off;
	uint32_t file_text_len;
	uint32_t file_data_len;
	uint8_t *tbase;
	uint8_t *dbase;
	uint8_t *bbase;
	uint32_t cmdlen;
	static char empty_env[] = { 0, 0 };

	if (!path || !path[0])
		return -1;

	handle = (uint16_t)Fopen(path, 0);
	if ((int16_t)handle < 0)
		return -1;

	if (mb_read_exact(handle, &hdr, sizeof(hdr)) != 0) {
		Fclose(handle);
		return -1;
	}

	if (hdr.magic != MB_PRG_MAGIC) {
		Fclose(handle);
		return -1;
	}

	text_off = MB_PRG_HDR_SIZE;
	entry_off = 0;
	file_text_len = hdr.tlen;
	file_data_len = hdr.dlen;
	if (hdr.res1 == MB_MINT_EXT_MAGIC) {
		if (Fseek((int32_t)MB_MINT_AOUT_OFF, handle, 0) < 0) {
			Fclose(handle);
			return -1;
		}
		if (mb_read_exact(handle, &aout, sizeof(aout)) != 0) {
			Fclose(handle);
			return -1;
		}
		if (aout.magic == 0x0107u || aout.magic == 0x0108u ||
		    aout.magic == 0x010bu) {
			entry_off = aout.entry;
			text_off = MB_MINT_TEXT_OFF;
			file_text_len = aout.text;
			file_data_len = aout.data;
			if (entry_off + file_text_len != hdr.tlen) {
				Fclose(handle);
				return -1;
			}
		}
	}

	text_data_len = hdr.tlen + hdr.dlen;
	total_len = (uint32_t)sizeof(*bp) + text_data_len + hdr.blen;
	tpa_start = mb_board_kernel_tpa_start();
	if (tpa_start == 0)
		tpa_start = MB_PRG_BASE_ADDR;
	tpa_end = *mb_lm_memtop();
	if (tpa_end == 0)
		tpa_end = *mb_lm_phystop();
	if (_mb_image_end && (uint32_t)(uintptr_t)_mb_image_end < tpa_end) {
		uint32_t end_addr = (uint32_t)(uintptr_t)_mb_image_end;
		end_addr = (end_addr + 3u) & ~3u;
		if (end_addr > tpa_start)
			tpa_start = end_addr;
	}
	if (tpa_end <= tpa_start + MB_PRG_STACK_RESERVE) {
		Fclose(handle);
		return -1;
	}

	if (tpa_start + total_len > tpa_end - MB_PRG_STACK_RESERVE) {
		Fclose(handle);
		return -1;
	}

	bp = (struct mb_basepage *)(uintptr_t)tpa_start;
	mb_kernel_last_basepage = bp;
	tbase = (uint8_t *)(bp + 1);
	dbase = tbase + hdr.tlen;
	bbase = dbase + hdr.dlen;
	mb_kernel_base = (uint32_t)(uintptr_t)tbase;
	mb_kernel_end = (uint32_t)(uintptr_t)(bbase + hdr.blen);

	memset(bp, 0, sizeof(*bp));
	bp->p_lowtpa = (uint8_t *)bp;
	bp->p_hitpa = (uint8_t *)(uintptr_t)tpa_end;
	bp->p_tbase = tbase;
	bp->p_tlen = hdr.tlen;
	bp->p_dbase = dbase;
	bp->p_dlen = hdr.dlen;
	bp->p_bbase = bbase;
	bp->p_blen = hdr.blen;
	bp->p_env = empty_env;

	cmdlen = 0;
	while (mb_cmdline[cmdlen] && cmdlen < MB_PDCLSIZE - 2)
		cmdlen++;
	bp->p_cmdlin[0] = (char)cmdlen;
	memcpy(&bp->p_cmdlin[1], mb_cmdline, cmdlen);
	bp->p_cmdlin[1 + cmdlen] = '\r';

	{
		struct mb_osheader *os = (struct mb_osheader *)mb_lm_oshdr();
		struct mb_basepage **runp = (struct mb_basepage **)mb_lm_osrun();

		memset(os, 0, sizeof(*os));
		*runp = bp;
		os->os_version = 0x0306u;
		os->os_beg = os;
		os->os_end = _mb_image_end ? (void *)_mb_image_end : (void *)os;
		os->p_run = runp;
		*mb_lm_sysbase() = os;
	}

	if (hdr.tlen)
		memset(tbase, 0, hdr.tlen);
	if (Fseek((int32_t)text_off, handle, 0) < 0) {
		Fclose(handle);
		return -1;
	}
	if (file_text_len) {
		long rc = mb_read_exact(handle, tbase + entry_off, file_text_len);
		if (rc != 0) {
			mb_log_printf("mintboot: load kernel text read failed rc=%ld\n", rc);
			Fclose(handle);
			return -1;
		}
	}
	if (file_data_len) {
		long rc = mb_read_exact(handle, dbase, file_data_len);
		if (rc != 0) {
			mb_log_printf("mintboot: load kernel data read failed rc=%ld\n", rc);
			Fclose(handle);
			return -1;
		}
	}
	if (hdr.blen)
		memset(bbase, 0, hdr.blen);

	if (hdr.abs == 0) {
		reloc_off = text_off + file_text_len + file_data_len + hdr.slen;
		mb_log_printf("mintboot: reloc off=%08x len=%08x\n",
			      reloc_off, text_data_len);
		if (mb_relocate_prg(handle, reloc_off, tbase, text_data_len) != 0) {
			mb_log_printf("mintboot: load kernel relocation failed\n");
			Fclose(handle);
			return -1;
		}
	}
	Fclose(handle);
	mb_log_printf("mintboot: load kernel success\n");
	{
		uint16_t *ins = (uint16_t *)(tbase + entry_off);
		mb_log_printf("mintboot: entry off=%08x ins=%04x %04x %04x %04x\n",
			      entry_off, ins[0], ins[1], ins[2], ins[3]);
	}

	if (do_jump) {
		void (*entry)(void *) = (void (*)(void *))(tbase + entry_off);

		mb_log_printf("mintboot: jump entry=%08x bp=%08x\n",
			      (uint32_t)(uintptr_t)entry,
			      (uint32_t)(uintptr_t)bp);
		mb_log_printf("mintboot: tpa=%08x..%08x memtop=%08x phystop=%08x membot=%08x\n",
			      tpa_start, tpa_end, *mb_lm_memtop(),
			      *mb_lm_phystop(), *mb_lm_membot());

		entry(bp);
	}

	return 0;
}
