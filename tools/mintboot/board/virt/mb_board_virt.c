#include "mintboot/mb_portable.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_cookie.h"
#include "mintboot/mb_lowmem.h"

#include <stdint.h>

void mb_virt_start(void);

#define VIRT_GF_TTY_MMIO_BASE 0xff008000u
#define VIRT_GF_CTRL_MMIO_BASE     0xff009000u
#define VIRT_GF_RTC_MMIO_BASE      0xff006000u
#define VIRT_GF_PIC_MMIO_BASE      0xff000000u
#define VIRT_GF_PIC_STRIDE         0x00001000u

#define GOLDFISH_TTY_PUT_CHAR      0x00
#define GOLDFISH_TTY_BYTES_READY   0x04
#define GOLDFISH_TTY_CMD           0x08
#define GOLDFISH_TTY_DATA_PTR      0x10
#define GOLDFISH_TTY_DATA_LEN      0x14
#define GOLDFISH_TTY_DATA_PTR_HIGH 0x18

#define GOLDFISH_CMD_WRITE_BUFFER  0x02
#define GOLDFISH_CMD_READ_BUFFER   0x03

#define GOLDFISH_CTRL_REG_CMD      0x04
#define GOLDFISH_CTRL_CMD_HALT     0x02

#define GOLDFISH_PIC_STATUS        0x00
#define GOLDFISH_PIC_ENABLE        0x04
#define GOLDFISH_PIC_DISABLE       0x08
#define GOLDFISH_PIC_ACK           0x0c

#define GOLDFISH_RTC_TIME_LOW      0x00
#define GOLDFISH_RTC_TIME_HIGH     0x04
#define GOLDFISH_RTC_ALARM_LOW     0x08
#define GOLDFISH_RTC_ALARM_HIGH    0x0c
#define GOLDFISH_RTC_IRQ_ENABLED   0x10
#define GOLDFISH_RTC_CLEAR_ALARM   0x14

static inline void mb_mmio_write32(uintptr_t addr, uint32_t value)
{
	*(volatile uint32_t *)addr = value;
}

static inline uint32_t mb_mmio_read32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static uint8_t mb_virt_rx;
static uint32_t mb_virt_tmr_ms = 20;
extern uint32_t mb_virt_bootinfo_ptr;

static struct mb_boot_info mb_virt_boot;

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

static void mb_virt_dump_bootinfo(void)
{
	const struct mb_linux_bootinfo *rec;
	uintptr_t addr;
	uint32_t idx;
	int saw_ramdisk = 0;

	extern uint8_t _mb_image_end[];

	if (mb_virt_bootinfo_ptr) {
		addr = (uintptr_t)mb_virt_bootinfo_ptr;
	} else {
		addr = (uintptr_t)_mb_image_end;
		addr = (addr + 3u) & ~3u;
	}
	rec = (const struct mb_linux_bootinfo *)addr;

	mb_log_printf("mintboot virt: bootinfo @ 0x%08x\r\n",
		      (uint32_t)addr);

	for (idx = 0; idx < 128; idx++) {
		if (rec->tag == 0) {
			mb_log_printf("mintboot virt: bootinfo end tag\r\n");
			break;
		}

		mb_log_printf("mintboot virt: bootinfo tag=0x%04x\r\n",
			      rec->tag);

		if (rec->tag == MB_LINUX_BI_MEMCHUNK) {
			const struct mb_linux_bootinfo_memchunk *mem;
			uint32_t end;

			if (rec->size < sizeof(*mem)) {
				mb_panic("bootinfo memchunk too small: %u",
					 rec->size);
			}

			mem = (const struct mb_linux_bootinfo_memchunk *)rec;
			if (mem->addr != 0) {
				mb_panic("bootinfo memchunk addr=0x%08x",
					 mem->addr);
			}

			end = mem->addr + mem->size_bytes;
			*mb_lm_phystop() = end;
			mb_log_printf("mintboot virt: phystop=0x%08x\r\n",
				      end);
		}

		if (rec->tag == MB_LINUX_BI_CPUTYPE) {
			const struct mb_linux_bootinfo_cputype *cpu;

			if (rec->size < sizeof(*cpu)) {
				mb_panic("bootinfo cputype too small: %u",
					 rec->size);
			}

			cpu = (const struct mb_linux_bootinfo_cputype *)rec;
			mb_log_printf("mintboot virt: cputype=%u\r\n",
				      cpu->cpu_type);
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
			mb_log_printf("mintboot virt: ramdisk=0x%08x size=0x%08x\r\n",
				      rd->addr, rd->size_bytes);
		}

		if (rec->size == 0) {
			mb_log_printf("mintboot virt: bootinfo size=0, stop\r\n");
			break;
		}

		rec = (const struct mb_linux_bootinfo *)((const uint8_t *)rec +
							 rec->size);
	}

	if (!saw_ramdisk)
		mb_panic("bootinfo missing ramdisk tag");
}
static long mb_virt_rom_bconout(uint16_t dev, uint16_t c)
{
	(void)dev;
	mb_board_console_putc((int)(c & 0xff));
	return 0;
}

static long mb_virt_rom_bconstat(uint16_t dev)
{
	(void)dev;
	return mb_board_console_poll() ? 1 : 0;
}

static long mb_virt_rom_bconin(uint16_t dev)
{
	(void)dev;
	return mb_board_console_getc();
}

const struct mb_rom_dispatch mb_rom_dispatch = {
	.fsetdta = mb_rom_fsetdta,
	.fgetdta = mb_rom_fgetdta,
	.dfree = mb_rom_dfree,
	.dcreate = mb_rom_dcreate,
	.ddelete = mb_rom_ddelete,
	.fcreate = mb_rom_fcreate,
	.fopen = mb_rom_fopen,
	.fclose = mb_rom_fclose,
	.fread = mb_rom_fread,
	.fwrite = mb_rom_fwrite,
	.fdelete = mb_rom_fdelete,
	.fseek = mb_rom_fseek,
	.fattrib = mb_rom_fattrib,
	.fsfirst = mb_rom_fsfirst,
	.fsnext = mb_rom_fsnext,
	.frename = mb_rom_frename,
	.fdatime = mb_rom_fdatime,
	.flock = mb_rom_flock,
	.fcntl = mb_rom_fcntl,
	.bconstat = mb_virt_rom_bconstat,
	.bconin = mb_virt_rom_bconin,
	.bconout = mb_virt_rom_bconout,
	.setexc = mb_rom_setexc,
	.bcostat = mb_rom_bcostat,
	.kbshift = mb_rom_kbshift,
	.initmous = mb_rom_initmous,
	.getrez = mb_rom_getrez,
	.iorec = mb_rom_iorec,
	.rsconf = mb_rom_rsconf,
	.keytbl = mb_rom_keytbl,
	.cursconf = mb_rom_cursconf,
	.settime = mb_rom_settime,
	.gettime = mb_rom_gettime,
	.bioskeys = mb_rom_bioskeys,
	.offgibit = mb_rom_offgibit,
	.ongibit = mb_rom_ongibit,
	.dosound = mb_rom_dosound,
	.kbdvbase = mb_rom_kbdvbase,
	.vsync = mb_rom_vsync,
	.bconmap = mb_rom_bconmap,
	.vsetscreen = mb_rom_vsetscreen,
	.kbrate = mb_rom_kbrate,
};

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
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_CLEAR_ALARM, 1);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_IRQ_ENABLED, 1);
}

static void mb_virt_enable_pic_irq(uint32_t pic, uint32_t irq)
{
	uintptr_t base = VIRT_GF_PIC_MMIO_BASE + (pic * VIRT_GF_PIC_STRIDE);

	mb_mmio_write32(base + GOLDFISH_PIC_DISABLE, 0xffffffffu);
	mb_mmio_write32(base + GOLDFISH_PIC_ENABLE, (1u << (irq - 1)));
	mb_mmio_write32(base + GOLDFISH_PIC_ACK, (1u << (irq - 1)));
}

static void mb_virt_puts(const char *s)
{
	for (; *s; s++)
		mb_board_console_putc(*s);
}

void mb_virt_start(void)
{
	mb_board_early_init();
	mb_portable_boot(&mb_virt_boot);

	mb_virt_puts("mintboot virt: mb_portable_boot returned.\r\n");
	mb_board_exit(0);
}

void mb_board_early_init(void)
{
	mb_virt_dump_bootinfo();
	*mb_lm_tmr_ms() = mb_virt_tmr_ms;
	mb_virt_enable_pic_irq(5, 1);
	mb_virt_init_rtc_50hz();
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

	low = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_LOW);
	high = mb_mmio_read32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_TIME_HIGH);
	now = ((uint64_t)high << 32) | low;
	alarm = now + ((uint64_t)mb_virt_tmr_ms * 1000000ull);
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_HIGH,
			(uint32_t)(alarm >> 32));
	mb_mmio_write32(VIRT_GF_RTC_MMIO_BASE + GOLDFISH_RTC_ALARM_LOW,
			(uint32_t)alarm);

	handler = (void (*)(void))(*mb_lm_etv_timer());
	if (handler)
		handler();
}

int mb_board_load_kernel(struct mb_boot_info *info)
{
	(void)info;
	/* TODO: load kernel image from virt storage or embedded image. */
	return -1;
}

void mb_board_console_putc(int ch)
{
	if (ch == '\n')
		mb_mmio_write32(VIRT_GF_TTY_MMIO_BASE + GOLDFISH_TTY_PUT_CHAR, '\r');
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
