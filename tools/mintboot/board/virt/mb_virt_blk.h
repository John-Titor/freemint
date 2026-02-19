#ifndef MINTBOOT_MB_VIRT_BLK_H
#define MINTBOOT_MB_VIRT_BLK_H

#include <stdint.h>

long mb_virt_blk_rwabs(uint16_t rwflag, void *buf, uint16_t count,
		       uint16_t recno, uint16_t dev);

#endif /* MINTBOOT_MB_VIRT_BLK_H */
