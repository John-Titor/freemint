#ifndef MINTBOOT_MB_LOWMEM_H
#define MINTBOOT_MB_LOWMEM_H

#include <stdint.h>

/*
 * Low-memory variables in the 0x400-0x5ff range used by the kernel.
 * Addresses follow standard TOS conventions.
 */
#define MB_LM_MEMVALID_ADDR   0x00000420u
#define MB_LM_PHYSTOP_ADDR    0x0000042eu
#define MB_LM_MEMVAL2_ADDR    0x0000043au
#define MB_LM_BOOTDEV_ADDR    0x00000446u
#define MB_LM_TMR_MS_ADDR     0x00000442u
#define MB_LM_ETV_CRITIC_ADDR 0x00000404u
#define MB_LM_ETV_TERM_ADDR   0x00000408u
#define MB_LM_ETV_TIMER_ADDR  0x0000040cu
#define MB_LM_HZ_200_ADDR     0x000004bau
#define MB_LM_DRVBITS_ADDR    0x000004c2u
#define MB_LM_COOKIE_P_ADDR   0x000005a0u

static inline volatile uint32_t *mb_lm_memvalid(void)
{
	return (volatile uint32_t *)MB_LM_MEMVALID_ADDR;
}

static inline volatile uint32_t *mb_lm_phystop(void)
{
	return (volatile uint32_t *)MB_LM_PHYSTOP_ADDR;
}

static inline volatile uint32_t *mb_lm_memval2(void)
{
	return (volatile uint32_t *)MB_LM_MEMVAL2_ADDR;
}

static inline volatile uint16_t *mb_lm_bootdev(void)
{
	return (volatile uint16_t *)MB_LM_BOOTDEV_ADDR;
}

static inline volatile uint16_t *mb_lm_tmr_ms(void)
{
	return (volatile uint16_t *)MB_LM_TMR_MS_ADDR;
}

static inline volatile uint32_t *mb_lm_etv_critic(void)
{
	return (volatile uint32_t *)MB_LM_ETV_CRITIC_ADDR;
}

static inline volatile uint32_t *mb_lm_etv_term(void)
{
	return (volatile uint32_t *)MB_LM_ETV_TERM_ADDR;
}

static inline volatile uint32_t *mb_lm_etv_timer(void)
{
	return (volatile uint32_t *)MB_LM_ETV_TIMER_ADDR;
}

static inline volatile uint32_t *mb_lm_hz_200(void)
{
	return (volatile uint32_t *)MB_LM_HZ_200_ADDR;
}

static inline volatile uint32_t *mb_lm_drvbits(void)
{
	return (volatile uint32_t *)MB_LM_DRVBITS_ADDR;
}

static inline volatile void **mb_lm_cookie_p(void)
{
	return (volatile void **)MB_LM_COOKIE_P_ADDR;
}

#endif /* MINTBOOT_MB_LOWMEM_H */
