#ifndef MINTBOOT_MB_ROM_H
#define MINTBOOT_MB_ROM_H

#include <stdint.h>

#include "mintboot/mb_errors.h"

#define MB_MAX_DRIVES 4u

struct mb_rom_dispatch {
	long (*bconstat)(uint16_t dev);
	long (*bconin)(uint16_t dev);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rwabs)(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno, uint16_t dev);
	long (*bcostat)(uint16_t dev);
	long (*drvmap)(void);

	long (*settime)(uint32_t time);
	long (*gettime)(void);
};

extern const struct mb_rom_dispatch mb_rom_dispatch;

long mb_bdos_fsetdta(void *dta);
void *mb_bdos_fgetdta(void);

long mb_bdos_dfree(uint32_t buf, uint16_t d);
long mb_bdos_dsetpath(const char *path);
long mb_bdos_dsetdrv(uint16_t drive);
long mb_bdos_dgetdrv(void);
void mb_bdos_set_current_drive(uint16_t drive);
uint16_t mb_bdos_get_current_drive(void);
const char *mb_bdos_get_current_path(uint16_t drive);
long mb_bdos_dgetpath(char *buf, uint16_t drive);
long mb_bdos_pterm0(void);
long mb_bdos_cconin(void);
long mb_bdos_cconws(const char *buf);
long mb_bdos_mxalloc(int32_t amount, uint16_t mode);
long mb_bdos_fcreate(const char *fn, uint16_t mode);
long mb_bdos_fopen(const char *filename, uint16_t mode);
long mb_bdos_fclose(uint16_t handle);
long mb_bdos_fread(uint16_t handle, uint32_t cnt, void *buf);
long mb_bdos_fwrite(uint16_t handle, uint32_t cnt, void *buf);
long mb_bdos_fdelete(const char *fn);
long mb_bdos_fseek(int32_t where, uint16_t handle, uint16_t how);
long mb_bdos_flock(uint16_t handle, uint16_t mode, int32_t start, int32_t length);
long mb_bdos_fcntl(uint16_t f, uint32_t arg, uint16_t cmd);
long mb_bdos_mshrink(uint16_t zero, uint32_t base, uint32_t len);

long mb_bios_bconstat(uint16_t dev);
long mb_bios_bconin(uint16_t dev);
long mb_bios_bconout(uint16_t dev, uint16_t c);
long mb_bios_rwabs(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno, uint16_t dev);
long mb_bios_setexc(uint16_t vnum, uint32_t vptr);
long mb_bios_getbpb(uint16_t dev);
long mb_bios_bcostat(uint16_t dev);
long mb_bios_drvmap(void);
long mb_bios_kbshift(uint16_t data);

long mb_xbios_initmous(uint16_t type, uint32_t param, uint32_t vptr);
long mb_xbios_physbase(void);
long mb_xbios_logbase(void);
long mb_xbios_getrez(void);
long mb_xbios_iorec(uint16_t io_dev);
long mb_xbios_rsconf(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs, uint16_t ts, uint16_t sc);
long mb_xbios_keytbl(uint32_t nrml, uint32_t shft, uint32_t caps);
long mb_xbios_cursconf(uint16_t rate, uint16_t attr);
long mb_xbios_settime(uint32_t time);
long mb_xbios_gettime(void);
long mb_xbios_bioskeys(void);
long mb_xbios_offgibit(uint16_t ormask);
long mb_xbios_ongibit(uint16_t andmask);
long mb_xbios_dosound(uint32_t ptr);
long mb_xbios_kbdvbase(void);
long mb_xbios_vsync(void);
long mb_xbios_supexec(uint32_t func);
long mb_xbios_bconmap(uint16_t dev);
long mb_xbios_vsetscreen(uint32_t lscrn, uint32_t pscrn, uint16_t rez, uint16_t mode);
long mb_xbios_kbrate(uint16_t delay, uint16_t rate);

long mb_bdos_dispatch(uint16_t fnum, uint16_t *args);
long mb_bios_dispatch(uint16_t fnum, uint16_t *args);
long mb_xbios_dispatch(uint16_t fnum, uint16_t *args);

#endif /* MINTBOOT_MB_ROM_H */
