#ifndef MINTBOOT_MB_TRAP_HELPERS_H
#define MINTBOOT_MB_TRAP_HELPERS_H

#include <stdint.h>

static inline uint32_t mb_arg32(uint32_t *args, int idx)
{
	return args[idx];
}

static inline uint16_t mb_arg16(uint32_t *args, int idx)
{
	return (uint16_t)args[idx];
}

#endif /* MINTBOOT_MB_TRAP_HELPERS_H */
