#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_board.h"

#include <stddef.h>
#include <string.h>

#define MB_PRG_MAGIC 0x601au
#define MB_PRG_HDR_SIZE 28u
#define MB_PRG_BASE_ADDR 0x00010000u
#define MB_PRG_STACK_RESERVE 0x00001000u

extern uint8_t _mb_image_end[] __attribute__((weak));

static struct mb_basepage *mb_kernel_last_basepage;
static uint32_t mb_kernel_base;
static uint32_t mb_kernel_end;

uint32_t mb_portable_last_basepage(void)
{
	return (uint32_t)(uintptr_t)mb_kernel_last_basepage;
}

void mb_portable_kernel_bounds(uint32_t *base, uint32_t *end)
{
	if (base)
		*base = mb_kernel_base;
	if (end)
		*end = mb_kernel_end;
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

static int mb_read_exact(uint16_t handle, void *buf, uint32_t len)
{
	uint32_t offset = 0;
	uint8_t *dst = (uint8_t *)buf;

	while (offset < len) {
		long rc = mb_rom_fread(handle, len - offset, dst + offset);
		if (rc <= 0)
			return -1;
		offset += (uint32_t)rc;
	}

	return 0;
}

static int mb_relocate_prg(uint16_t handle, uint8_t *tbase,
			   uint32_t text_data_len, uint32_t slen)
{
	uint32_t relst = 0;
	uint32_t offset = 0;
	uint8_t buf[256];

	if (mb_rom_fseek((int32_t)(MB_PRG_HDR_SIZE + text_data_len + slen),
			 handle, 0) < 0)
		return -1;

	if (mb_read_exact(handle, &relst, sizeof(relst)) != 0)
		return -1;

	if (relst == 0)
		return 0;

	mb_log_printf("mintboot: PRG relocation base=%08x\r\n",
		      (uint32_t)(uintptr_t)tbase);

	offset = relst;
	if (offset >= text_data_len)
		return -1;
	*(uint32_t *)(tbase + offset) += (uint32_t)(uintptr_t)tbase;

	for (;;) {
		long rc = mb_rom_fread(handle, sizeof(buf), buf);
		uint32_t i;

		if (rc < 0)
			return -1;
		if (rc == 0)
			break;

		for (i = 0; i < (uint32_t)rc; i++) {
			uint8_t b = buf[i];
			if (b == 0)
				return 0;
			if (b == 1)
				offset += 254;
			else
				offset += b;
			if (offset >= text_data_len)
				return -1;
			*(uint32_t *)(tbase + offset) += (uint32_t)(uintptr_t)tbase;
		}
	}

	return -1;
}

int mb_portable_load_kernel(const char *path, int do_jump)
{
	struct mb_prg_header hdr;
	struct mb_basepage *bp;
	uint16_t handle;
	uint32_t text_data_len;
	uint32_t total_len;
	uint32_t tpa_start;
	uint32_t tpa_end;
	uint32_t stack_top;
	uint8_t *tbase;
	uint8_t *dbase;
	uint8_t *bbase;
	uint32_t cmdlen;
	static char empty_env[] = { 0, 0 };

	if (!path || !path[0])
		return -1;

	handle = (uint16_t)mb_rom_fopen(path, 0);
	if ((int16_t)handle < 0)
		return -1;

	if (mb_read_exact(handle, &hdr, sizeof(hdr)) != 0) {
		mb_rom_fclose(handle);
		return -1;
	}

	if (hdr.magic != MB_PRG_MAGIC) {
		mb_rom_fclose(handle);
		return -1;
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
		mb_rom_fclose(handle);
		return -1;
	}

	stack_top = tpa_end - MB_PRG_STACK_RESERVE;
	if (tpa_start + total_len > stack_top) {
		mb_rom_fclose(handle);
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
	bp->p_hitpa = (uint8_t *)(uintptr_t)stack_top;
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

	if (mb_read_exact(handle, tbase, text_data_len) != 0) {
		mb_rom_fclose(handle);
		return -1;
	}

	if (hdr.blen)
		memset(bbase, 0, hdr.blen);

	if (hdr.abs == 0) {
		if (mb_relocate_prg(handle, tbase, text_data_len, hdr.slen) != 0) {
			mb_rom_fclose(handle);
			return -1;
		}
	}

	mb_rom_fclose(handle);

	if (do_jump) {
		void (*entry)(void) = (void (*)(void))tbase;
		uint32_t sp = stack_top & ~3u;
		asm volatile(
			"move.l %0,%%a0\n\t"
			"move.l %1,%%sp\n\t"
			"jmp (%2)\n\t"
			:
			: "r"(bp), "r"(sp), "a"(entry)
			: "a0");
	}

	return 0;
}
