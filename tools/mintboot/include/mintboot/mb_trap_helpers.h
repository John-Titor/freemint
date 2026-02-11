#ifndef MINTBOOT_MB_TRAP_HELPERS_H
#define MINTBOOT_MB_TRAP_HELPERS_H

#include <stdint.h>

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

#endif /* MINTBOOT_MB_TRAP_HELPERS_H */
