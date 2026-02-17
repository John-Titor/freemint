#ifndef MINTBOOT_MB_BDOS_MEM_H
#define MINTBOOT_MB_BDOS_MEM_H

#include <stdint.h>

#define MB_MEM_POOL_ST 0u
#define MB_MEM_POOL_TT 1u

void mb_bdos_mem_set_st_ram(uint32_t base, uint32_t size);
void mb_bdos_mem_set_tt_ram(uint32_t base, uint32_t size);
long mb_bdos_mem_malloc(int32_t amount);
long mb_bdos_mem_mxalloc(int32_t amount, uint16_t mode);
long mb_bdos_mem_mfree(uint32_t base);
long mb_bdos_mem_mshrink(uint16_t zero, uint32_t base, uint32_t len);

#endif /* MINTBOOT_MB_BDOS_MEM_H */
