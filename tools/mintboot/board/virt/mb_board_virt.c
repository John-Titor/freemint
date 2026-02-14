#include "mintboot/mb_portable.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_cookie.h"
#include "mintboot/mb_lowmem.h"
#include "mb_virt_mmio.h"

#include <stdint.h>

void mb_virt_start(void);

static uint8_t mb_virt_rx;
static const uint32_t mb_virt_tmr_ms = 20;
static const uint32_t mb_virt_hz200_step = 4;

struct mb_boot_info mb_virt_boot;

struct mb_linux_bootinfo {
	uint16_t tag;
	uint16_t size;
};

struct mb_linux_bootinfo_memchunk {
	uint16_t tag;
	uint16_t size;
	uint32_t addr;
	uint32_t size_bytes;
};

#define MB_LINUX_BI_MEMCHUNK 0x0005
#define MB_LINUX_BI_CPUTYPE  0x0003
#define MB_LINUX_BI_RAMDISK  0x0006
#define MB_LINUX_BI_CMDLINE  0x0007

struct mb_linux_bootinfo_cputype {
	uint16_t tag;
	uint16_t size;
	uint32_t cpu_type;
};

struct mb_linux_bootinfo_ramdisk {
	uint16_t tag;
	uint16_t size;
	uint32_t addr;
	uint32_t size_bytes;
};

static void mb_virt_parse_bootinfo(void)
{
	const struct mb_linux_bootinfo *rec;
	uintptr_t addr;
	uint32_t idx;
	int saw_ramdisk = 0;

	extern uint8_t _mb_image_end[];

	addr = (uintptr_t)_mb_image_end;
	addr = (addr + 3u) & ~3u;
	rec = (const struct mb_linux_bootinfo *)addr;

	/* mb_log_printf("mintboot virt: bootinfo @ 0x%08x\n",
		      (uint32_t)addr); */

	for (idx = 0; idx < 128; idx++) {
		if (rec->tag == 0) {
			/* mb_log_printf("mintboot virt: bootinfo end tag\n"); */
			break;
		}

		/* mb_log_printf("mintboot virt: bootinfo tag=0x%04x\n",
			      rec->tag); */

		if (rec->tag == MB_LINUX_BI_MEMCHUNK) {
			const struct mb_linux_bootinfo_memchunk *mem;
			uint32_t st_size;

			if (rec->size < sizeof(*mem)) {
				mb_panic("bootinfo memchunk too small: %u",
					 rec->size);
			}

			mem = (const struct mb_linux_bootinfo_memchunk *)rec;
			if (mem->addr != 0) {
				mb_panic("bootinfo memchunk addr=0x%08x",
					 mem->addr);
			}

			if (mem->size_bytes < 0x04000000u) {
				mb_panic("bootinfo memchunk too small: %u",
					 mem->size_bytes);
			}

			st_size = 0x03f00000u;
			mb_portable_set_st_ram(mem->addr, st_size);
		}

		if (rec->tag == MB_LINUX_BI_CPUTYPE) {
			const struct mb_linux_bootinfo_cputype *cpu;

			if (rec->size < sizeof(*cpu)) {
				mb_panic("bootinfo cputype too small: %u",
					 rec->size);
			}

			cpu = (const struct mb_linux_bootinfo_cputype *)rec;
			/* mb_log_printf("mintboot virt: cputype=%u\n",
				      cpu->cpu_type); */
			if (cpu->cpu_type != 4) {
				mb_panic("bootinfo cputype=%u expected=4",
					 cpu->cpu_type);
			}
		}

		if (rec->tag == MB_LINUX_BI_RAMDISK) {
			const struct mb_linux_bootinfo_ramdisk *rd;

			if (rec->size < sizeof(*rd)) {
				mb_panic("bootinfo ramdisk too small: %u",
					 rec->size);
			}

			rd = (const struct mb_linux_bootinfo_ramdisk *)rec;
			mb_virt_boot.ramdisk_base = (void *)(uintptr_t)rd->addr;
			mb_virt_boot.ramdisk_size = rd->size_bytes;
			saw_ramdisk = 1;
			/* mb_log_printf("mintboot virt: ramdisk=0x%08x size=0x%08x\n",
				      rd->addr, rd->size_bytes); */
		}

		if (rec->tag == MB_LINUX_BI_CMDLINE) {
			uint32_t len;
			uint32_t i;
			const uint8_t *src;

			if (rec->size < sizeof(*rec)) {
				mb_panic("bootinfo cmdline too small: %u",
					 rec->size);
			}

			len = rec->size - (uint32_t)sizeof(*rec);
			if (len > sizeof(mb_cmdline) - 1)
				len = sizeof(mb_cmdline) - 1;

			src = (const uint8_t *)(rec + 1);
			for (i = 0; i < len; i++)
				mb_cmdline[i] = (char)src[i];
			mb_cmdline[len] = '\0';
		}

		if (rec->size == 0) {
			/* mb_log_printf("mintboot virt: bootinfo size=0, stop\n"); */
			break;
		}

		rec = (const struct mb_linux_bootinfo *)((const uint8_t *)rec +
							 rec->size);
	}

	if (!saw_ramdisk)
		mb_panic("bootinfo missing ramdisk tag");
}
static void mb_virt_init_rtc_50hz(void)
{
	uint64_t now;
	uint64_t alarm;
	uint32_t low;
	uint32_t high;

	low = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_LOW);
	high = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_HIGH);
	now = ((uint64_t)high << 32) | low;

	alarm = now + ((uint64_t)mb_virt_tmr_ms * 1000000ull);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_HIGH,
			(uint32_t)(alarm >> 32));
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_LOW,
			(uint32_t)alarm);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_IRQ_ENABLED, 1);
}

static void mb_virt_enable_pic_irq(uint32_t pic, uint32_t irq)
{
	uintptr_t base;

	if (pic < 1u || pic > 6u || irq < 1u || irq > 32u)
		mb_panic("virt pic cfg out of range pic=%u irq=%u", pic, irq);

	base = VIRT_GF_PIC_MMIO_BASE + ((pic - 1u) * VIRT_GF_PIC_STRIDE);

	mb_mmio_write32(base + GOLDFISH_PIC_ENABLE, (1u << (irq - 1u)));
}

void mb_virt_start(void)
{
	mb_board_early_init();
	mb_portable_boot(&mb_virt_boot);
	mb_board_exit(0);
}

void mb_board_early_init(void)
{
	mb_virt_parse_bootinfo();
	*mb_lm_tmr_ms() = mb_virt_tmr_ms;
	mb_virt_enable_pic_irq(6, 1);
	mb_virt_init_rtc_50hz();
}

void mb_board_init_cookies(void)
{
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'C', 'P', 'U'), 40);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'F', 'P', 'U'), 40);
}

void mb_autovec_level6_handler(void)
{
	uint64_t now;
	uint64_t alarm;
	uint32_t low;
	uint32_t high;
	void (*handler)(void);

	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_CLEAR_ALARM, 1);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_CLEAR_INTERRUPT, 1);

	low = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_LOW);
	high = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_HIGH);
	now = ((uint64_t)high << 32) | low;
	alarm = now + ((uint64_t)mb_virt_tmr_ms * 1000000ull);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_HIGH,
			(uint32_t)(alarm >> 32));
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_LOW,
			(uint32_t)alarm);

	*mb_lm_hz_200() = *mb_lm_hz_200() + mb_virt_hz200_step;

	handler = (void (*)(void))(*mb_lm_etv_timer());
	if (handler)
		handler();
}

void mb_board_console_putc(int ch)
{
	mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_PUT_CHAR,
			(uint32_t)ch);
}

int mb_board_console_getc(void)
{
	uintptr_t paddr = (uintptr_t)&mb_virt_rx;

	while (!mb_board_console_poll()) {
	}

	mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_DATA_PTR,
			(uint32_t)paddr);
	mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_DATA_LEN, 1);
	mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_DATA_PTR_HIGH, 0);
	mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_CMD,
			GOLDFISH_CMD_READ_BUFFER);

	return mb_virt_rx;
}

int mb_board_console_poll(void)
{
	return mb_mmio_read32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_BYTES_READY) != 0;
}

void mb_board_exit(int code)
{
	(void)code;
	mb_mmio_write32(VIRT_GF_CTRL_MMIO_BASE + GOLDFISH_CTRL_REG_CMD,
			GOLDFISH_CTRL_CMD_HALT);
	for (;;) {
	}
}
