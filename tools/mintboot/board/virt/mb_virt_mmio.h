#ifndef MINTBOOT_MB_VIRT_MMIO_H
#define MINTBOOT_MB_VIRT_MMIO_H

#include <stdint.h>

#define VIRT_GF_TTY_MMIO_BASE  0xff008000u
#define VIRT_GF_CTRL_MMIO_BASE 0xff009000u
#define VIRT_GF_RTC_MMIO_BASE  0xff006000u
#define VIRT_GF_PIC_MMIO_BASE  0xff000000u
#define VIRT_GF_PIC_STRIDE     0x00001000u

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
#define GOLDFISH_CTRL_CMD_PANIC    0x03

#define GOLDFISH_PIC_STATUS        0x00
#define GOLDFISH_PIC_NUMBER        0x04
#define GOLDFISH_PIC_DISABLE_ALL   0x08
#define GOLDFISH_PIC_DISABLE       0x0c
#define GOLDFISH_PIC_ENABLE        0x10

#define GOLDFISH_RTC_TIME_LOW      0x00
#define GOLDFISH_RTC_TIME_HIGH     0x04
#define GOLDFISH_RTC_ALARM_LOW     0x08
#define GOLDFISH_RTC_ALARM_HIGH    0x0c
#define GOLDFISH_RTC_IRQ_ENABLED   0x10
#define GOLDFISH_RTC_CLEAR_ALARM   0x14
#define GOLDFISH_RTC_ALARM_STATUS  0x18
#define GOLDFISH_RTC_CLEAR_INTERRUPT 0x1c

static inline void mb_mmio_write32(uintptr_t addr, uint32_t value)
{
	*(volatile uint32_t *)addr = value;
}

static inline uint32_t mb_mmio_read32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

#endif /* MINTBOOT_MB_VIRT_MMIO_H */
