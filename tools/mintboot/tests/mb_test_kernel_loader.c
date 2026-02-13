#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mb_tests_internal.h"

#include <string.h>

#ifndef str
#define str(x) _stringify(x)
#define _stringify(x) #x
#endif
#include "../../../sys/buildinfo/version.h"

#define MB_MINT_EXT_MAGIC 0x4d694e54u

struct mb_test_prg_header {
	uint16_t magic;
	uint32_t tlen;
	uint32_t dlen;
	uint32_t blen;
	uint32_t slen;
	uint32_t res1;
	uint32_t flags;
	uint16_t abs;
} __attribute__((packed));

struct mb_test_aout_header {
	uint32_t magic;
	uint32_t text;
	uint32_t data;
	uint32_t bss;
	uint32_t syms;
	uint32_t entry;
	uint32_t trsize;
	uint32_t drsize;
} __attribute__((packed));

struct mb_test_basepage {
	uint8_t *p_lowtpa;
	uint8_t *p_hitpa;
	uint8_t *p_tbase;
	uint32_t p_tlen;
	uint8_t *p_dbase;
	uint32_t p_dlen;
	uint8_t *p_bbase;
	uint32_t p_blen;
	void *p_xdta;
	struct mb_test_basepage *p_parent;
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

static uint32_t mb_read_u32_at(uint16_t handle, uint32_t off)
{
	uint32_t v;

	if (Fseek((int32_t)off, handle, 0) < 0)
		mb_panic("Reloc test: seek failed off=%08x", off);
	if (Fread(handle, sizeof(v), &v) != (long)sizeof(v))
		mb_panic("Reloc test: read failed off=%08x", off);
	return v;
}

static uint32_t mb_read_unrelocated_u32(uint16_t handle, uint32_t text_off,
					uint32_t entry_off, uint32_t file_text_len,
					uint32_t file_data_len, uint32_t hdr_tlen,
					uint32_t logical_off)
{
	uint32_t file_off;

	if (logical_off >= entry_off &&
	    logical_off + 4u <= entry_off + file_text_len) {
		file_off = text_off + (logical_off - entry_off);
		return mb_read_u32_at(handle, file_off);
	}

	if (logical_off >= hdr_tlen &&
	    logical_off + 4u <= hdr_tlen + file_data_len) {
		file_off = text_off + file_text_len + (logical_off - hdr_tlen);
		return mb_read_u32_at(handle, file_off);
	}

	mb_panic("Reloc test: unsupported logical off=%08x", logical_off);
	return 0;
}

static void mb_tests_kernel_mint_ext_reloc(const char *kernel_path, uint8_t *tbase)
{
	struct mb_test_prg_header hdr;
	struct mb_test_aout_header aout;
	uint16_t handle;
	uint32_t text_off;
	uint32_t entry_off;
	uint32_t file_text_len;
	uint32_t file_data_len;
	uint32_t reloc_off;
	uint32_t relst;
	uint32_t relbase;
	uint32_t offset;
	uint32_t ext_off;
	uint32_t next_off;
	uint32_t file_ext;
	uint32_t file_next;
	uint32_t got_ext;
	uint32_t got_next;
	uint8_t reloc_buf[256];
	int saw_ext;

	handle = (uint16_t)Fopen(kernel_path, 0);
	if ((int16_t)handle < 0)
		mb_panic("Reloc test: open failed");
	if (Fread(handle, sizeof(hdr), &hdr) != (long)sizeof(hdr))
		mb_panic("Reloc test: header read");

	if (hdr.res1 != MB_MINT_EXT_MAGIC)
		mb_panic("Reloc test: kernel is not MiNT extended PRG");

	text_off = 0x100u;
	entry_off = 0;
	file_text_len = hdr.tlen;
	file_data_len = hdr.dlen;

	if (Fseek(0x24, handle, 0) < 0)
		mb_panic("Reloc test: aout seek");
	if (Fread(handle, sizeof(aout), &aout) != (long)sizeof(aout))
		mb_panic("Reloc test: aout read");
	if (aout.magic == 0x0107u || aout.magic == 0x0108u ||
	    aout.magic == 0x010bu) {
		entry_off = aout.entry;
		file_text_len = aout.text;
		file_data_len = aout.data;
	}

	reloc_off = text_off + file_text_len + file_data_len + hdr.slen;
	if (Fseek((int32_t)reloc_off, handle, 0) < 0)
		mb_panic("Reloc test: reloc seek");
	if (Fread(handle, sizeof(relst), &relst) != (long)sizeof(relst))
		mb_panic("Reloc test: relst read");
	if (relst == 0)
		mb_panic("Reloc test: empty relocation table");

	offset = relst;
	ext_off = 0;
	next_off = 0;
	saw_ext = 0;
	for (;;) {
		long rc = Fread(handle, sizeof(reloc_buf), reloc_buf);
		uint32_t i;

		if (rc < 0)
			mb_panic("Reloc test: reloc stream read");
		if (rc == 0)
			break;

		for (i = 0; i < (uint32_t)rc; i++) {
			uint8_t b = reloc_buf[i];
			if (b == 0) {
				i = (uint32_t)rc;
				break;
			}
			if (!saw_ext) {
				if (b == 1) {
					offset += 254u;
					ext_off = offset;
					saw_ext = 1;
					continue;
				}
				offset += (uint32_t)b;
				continue;
			}
			if (b == 1) {
				offset += 254u;
				continue;
			}
			offset += (uint32_t)b;
			next_off = offset;
			i = (uint32_t)rc;
			break;
		}
		if (next_off != 0)
			break;
	}

	if (!saw_ext || next_off == 0)
		mb_panic("Reloc test: could not find extension case");

	file_ext = mb_read_unrelocated_u32(handle, text_off, entry_off, file_text_len,
					   file_data_len, hdr.tlen, ext_off);
	file_next = mb_read_unrelocated_u32(handle, text_off, entry_off, file_text_len,
					    file_data_len, hdr.tlen, next_off);
	Fclose(handle);

	memcpy(&got_ext, tbase + ext_off, sizeof(got_ext));
	memcpy(&got_next, tbase + next_off, sizeof(got_next));
	relbase = (uint32_t)(uintptr_t)tbase;

	if (got_ext != file_ext)
		mb_panic("Reloc test: ext off=%08x got=%08x file=%08x",
			 ext_off, got_ext, file_ext);
	if (got_next != file_next + relbase)
		mb_panic("Reloc test: next off=%08x got=%08x exp=%08x",
			 next_off, got_next, file_next + relbase);
}

void mb_tests_kernel_loader(void)
{
	static const int do_jump = 0;
	char kernel_path[96];
	struct mb_test_prg_header hdr;
	struct mb_test_aout_header aout;
	uint16_t handle;
	long rc;
	uint32_t bp_addr;
	struct mb_test_basepage *bp;
	char drive;
	uint16_t dev;
	uint32_t tlen;
	uint32_t dlen;
	uint32_t blen;

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

	/*
	 * Loader test must not transfer control to the kernel; we only validate
	 * load/relocation and basepage state here.
	 */
	rc = mb_portable_load_kernel(kernel_path, do_jump);
	if (rc != 0)
		mb_panic("Kernel test: load rc=%d", (int)rc);

	bp_addr = mb_portable_last_basepage();
	if (bp_addr == 0)
		mb_panic("Kernel test: basepage missing");
	bp = (struct mb_test_basepage *)(uintptr_t)bp_addr;
	if (bp->p_tlen != tlen || bp->p_dlen != dlen || bp->p_blen != blen)
		mb_panic("Kernel test: len mismatch");
	if (bp->p_tbase != (uint8_t *)(bp + 1))
		mb_panic("Kernel test: tbase mismatch");
	if (bp->p_parent || bp->p_xdta)
		mb_panic("Kernel test: basepage parent/dta not null");

	mb_tests_kernel_mint_ext_reloc(kernel_path, bp->p_tbase);
}
