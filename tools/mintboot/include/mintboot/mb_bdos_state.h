#ifndef MINTBOOT_MB_GEMDOS_STATE_H
#define MINTBOOT_MB_GEMDOS_STATE_H

#include <stdint.h>

int mb_bdos_set_current_path(uint16_t drive, const char *path);
void mb_bdos_set_dta(void *dta);
void *mb_bdos_get_dta(void);

#endif /* MINTBOOT_MB_GEMDOS_STATE_H */
