#ifndef MINTBOOT_MB_DEBUG_H
#define MINTBOOT_MB_DEBUG_H

#include <stdint.h>

void mb_debug_bios_enter(uint16_t fnum, uint16_t *args);
void mb_debug_bios_exit(uint16_t fnum, uint16_t *args, long ret);
void mb_debug_xbios_enter(uint16_t fnum, uint16_t *args);
void mb_debug_xbios_exit(uint16_t fnum, uint16_t *args, long ret);
void mb_debug_bdos_enter(uint16_t fnum, uint16_t *args);
void mb_debug_bdos_exit(uint16_t fnum, uint16_t *args, long ret);
void mb_debug_vdi_enter(uint16_t fnum, uint16_t *args);
void mb_debug_vdi_exit(uint16_t fnum, uint16_t *args, long ret);
void mb_debug_timer_tick(void);

#endif /* MINTBOOT_MB_DEBUG_H */
