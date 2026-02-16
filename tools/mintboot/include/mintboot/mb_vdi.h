#ifndef MINTBOOT_MB_VDI_H
#define MINTBOOT_MB_VDI_H

#include <stdint.h>

long mb_vdi_dispatch(uint16_t function, uint16_t *args, uint32_t *retaddr);

#endif /* MINTBOOT_MB_VDI_H */
