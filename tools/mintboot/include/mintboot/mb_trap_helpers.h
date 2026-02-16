#ifndef MINTBOOT_MB_TRAP_HELPERS_H
#define MINTBOOT_MB_TRAP_HELPERS_H

#include <stdint.h>
#include "mintboot/mb_common.h"
#include "mintboot/mb_lowmem.h"

extern uint8_t _mb_image_start[] __attribute__((weak));
extern uint8_t _mb_image_end[] __attribute__((weak));

static inline uint32_t mb_arg32(const uint16_t *args, int idx)
{
	uint32_t hi = (uint32_t)args[idx * 2];
	uint32_t lo = (uint32_t)args[idx * 2 + 1];

	return (hi << 16) | lo;
}

static inline uint16_t mb_arg16(const uint16_t *args, int idx)
{
	return args[idx];
}

static inline uint32_t mb_arg32w(const uint16_t *args, int word_idx)
{
	uint32_t hi = (uint32_t)args[word_idx];
	uint32_t lo = (uint32_t)args[word_idx + 1];

	return (hi << 16) | lo;
}

static inline int mb_is_sane_vector_target(uint32_t ptr)
{
	uint32_t st_lo = *mb_lm_membot();
	uint32_t st_hi = *mb_lm_phystop();
	uint32_t mb_lo;
	uint32_t mb_hi;

	if ((ptr & 1u) != 0u)
		return 0;

	if (_mb_image_start && _mb_image_end) {
		mb_lo = (uint32_t)(uintptr_t)_mb_image_start;
		mb_hi = (uint32_t)(uintptr_t)_mb_image_end;
	} else {
		mb_lo = (uint32_t)(uintptr_t)mb_common_set_st_ram;
		mb_hi = mb_lo;
	}

	if ((mb_hi > mb_lo) && (ptr >= mb_lo) && (ptr < mb_hi))
		return 1;
	if ((st_hi > st_lo) && (ptr >= st_lo) && (ptr < st_hi))
		return 1;

	return 0;
}

static inline void mb_check_vector20(const char *who)
{
	uint32_t v = *(volatile uint32_t *)(uintptr_t)0x20u;

	if (!mb_is_sane_vector_target(v)) {
		mb_panic("%s: vector[8 @ 0x20] corrupted: %08x", who, v);
	}
}

#endif /* MINTBOOT_MB_TRAP_HELPERS_H */
