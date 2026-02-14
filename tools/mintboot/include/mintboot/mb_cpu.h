#ifndef MINTBOOT_MB_CPU_H
#define MINTBOOT_MB_CPU_H

#include <stdint.h>

static inline uint32_t mb_cpu_get_sp(void)
{
	uint32_t sp;

	__asm__ volatile("move.l %%sp, %0" : "=d"(sp));
	return sp;
}

static inline void mb_cpu_set_sp(uint32_t sp)
{
	__asm__ volatile("move.l %0, %%sp" : : "d"(sp) : "memory");
}

static inline uint32_t mb_cpu_get_usp(void)
{
	uint32_t usp;

	__asm__ volatile("move.l %%usp, %0" : "=a"(usp));
	return usp;
}

static inline void mb_cpu_set_usp(uint32_t usp)
{
	__asm__ volatile("move.l %0, %%usp" : : "a"(usp) : "memory");
}

static inline uint16_t mb_cpu_read_sr(void)
{
	uint16_t sr;

	__asm__ volatile("move.w %%sr, %0" : "=d"(sr));
	return sr;
}

static inline void mb_cpu_write_sr(uint16_t sr)
{
	__asm__ volatile("move.w %0, %%sr" : : "d"(sr) : "cc", "memory");
}

static inline void mb_cpu_enable_interrupts(void)
{
	/* supervisor mode, interrupt mask = 0 */
	mb_cpu_write_sr(0x2000u);
}

static inline void mb_cpu_set_vbr(uint32_t vbr)
{
	__asm__ volatile("movec %0, %%vbr" : : "r"(vbr) : "memory");
}

static inline uint32_t mb_cpu_get_vbr(void)
{
	uint32_t vbr;

	__asm__ volatile("movec %%vbr, %0" : "=r"(vbr));
	return vbr;
}

static inline void mb_cpu_set_msp(uint32_t sp)
{
	__asm__ volatile("movec %0, %%msp" : : "r"(sp) : "memory");
}

static inline void mb_cpu_set_isp(uint32_t sp)
{
	__asm__ volatile("movec %0, %%isp" : : "r"(sp) : "memory");
}

#endif /* MINTBOOT_MB_CPU_H */
