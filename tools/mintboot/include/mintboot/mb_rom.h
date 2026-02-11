#ifndef MINTBOOT_MB_ROM_H
#define MINTBOOT_MB_ROM_H

#include <stdint.h>

#define MB_ERR_FNF (-33)
#define MB_ERR_PTH (-34)
#define MB_ERR_ACCDN (-36)
#define MB_ERR_XDEV (-48)
#define MB_ERR_NMFIL (-49)
#define MB_ERR_LOCKED (-58)
#define MB_ERR_EXIST (-85)
#define MB_ERR_BADF (-32)

struct mb_rom_dispatch {
	long (*fsetdta)(void *dta);
	void *(*fgetdta)(void);

	long (*dfree)(uint32_t buf, uint16_t d);
	long (*dcreate)(const char *path);
	long (*ddelete)(const char *path);
	long (*fcreate)(const char *fn, uint16_t mode);
	long (*fopen)(const char *filename, uint16_t mode);
	long (*fclose)(uint16_t handle);
	long (*fread)(uint16_t handle, uint32_t cnt, void *buf);
	long (*fwrite)(uint16_t handle, uint32_t cnt, void *buf);
	long (*fdelete)(const char *fn);
	long (*fseek)(int32_t where, uint16_t handle, uint16_t how);
	long (*fattrib)(const char *fn, uint16_t rwflag, uint16_t attr);
	long (*fsfirst)(const char *filespec, uint16_t attr);
	long (*fsnext)(void);
	long (*frename)(uint16_t zero, const char *oldname, const char *newname);
	long (*fdatime)(uint32_t timeptr, uint16_t handle, uint16_t rwflag);
	long (*flock)(uint16_t handle, uint16_t mode, int32_t start, int32_t length);
	long (*fcntl)(uint16_t f, uint32_t arg, uint16_t cmd);

	long (*bconstat)(uint16_t dev);
	long (*bconin)(uint16_t dev);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rwabs)(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno, uint16_t dev);
	long (*setexc)(uint16_t vnum, uint32_t vptr);
	long (*getbpb)(uint16_t dev);
	long (*bcostat)(uint16_t dev);
	long (*drvmap)(void);
	long (*kbshift)(uint16_t data);

	long (*initmous)(uint16_t type, uint32_t param, uint32_t vptr);
	long (*getrez)(void);
	long (*iorec)(uint16_t io_dev);
	long (*rsconf)(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs, uint16_t ts, uint16_t sc);
	long (*keytbl)(uint32_t nrml, uint32_t shft, uint32_t caps);
	long (*cursconf)(uint16_t rate, uint16_t attr);
	long (*settime)(uint32_t time);
	long (*gettime)(void);
	long (*bioskeys)(void);
	long (*offgibit)(uint16_t ormask);
	long (*ongibit)(uint16_t andmask);
	long (*dosound)(uint32_t ptr);
	long (*kbdvbase)(void);
	long (*vsync)(void);
	long (*bconmap)(uint16_t dev);
	long (*vsetscreen)(uint32_t lscrn, uint32_t pscrn, uint16_t rez, uint16_t mode);
	long (*kbrate)(uint16_t delay, uint16_t rate);
};

extern const struct mb_rom_dispatch mb_rom_dispatch;

long mb_rom_fsetdta(void *dta);
void *mb_rom_fgetdta(void);

long mb_rom_dfree(uint32_t buf, uint16_t d);
long mb_rom_dcreate(const char *path);
long mb_rom_ddelete(const char *path);
long mb_rom_fcreate(const char *fn, uint16_t mode);
long mb_rom_fopen(const char *filename, uint16_t mode);
long mb_rom_fclose(uint16_t handle);
long mb_rom_fread(uint16_t handle, uint32_t cnt, void *buf);
long mb_rom_fwrite(uint16_t handle, uint32_t cnt, void *buf);
long mb_rom_fdelete(const char *fn);
long mb_rom_fseek(int32_t where, uint16_t handle, uint16_t how);
long mb_rom_fattrib(const char *fn, uint16_t rwflag, uint16_t attr);
long mb_rom_fsfirst(const char *filespec, uint16_t attr);
long mb_rom_fsnext(void);
long mb_rom_frename(uint16_t zero, const char *oldname, const char *newname);
long mb_rom_fdatime(uint32_t timeptr, uint16_t handle, uint16_t rwflag);
long mb_rom_flock(uint16_t handle, uint16_t mode, int32_t start, int32_t length);
long mb_rom_fcntl(uint16_t f, uint32_t arg, uint16_t cmd);

long mb_rom_bconstat(uint16_t dev);
long mb_rom_bconin(uint16_t dev);
long mb_rom_bconout(uint16_t dev, uint16_t c);
long mb_rom_rwabs(uint16_t rwflag, void *buf, uint16_t count, uint16_t recno, uint16_t dev);
long mb_rom_setexc(uint16_t vnum, uint32_t vptr);
long mb_rom_getbpb(uint16_t dev);
long mb_rom_bcostat(uint16_t dev);
long mb_rom_drvmap(void);
long mb_rom_kbshift(uint16_t data);

long mb_rom_initmous(uint16_t type, uint32_t param, uint32_t vptr);
long mb_rom_getrez(void);
long mb_rom_iorec(uint16_t io_dev);
long mb_rom_rsconf(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs, uint16_t ts, uint16_t sc);
long mb_rom_keytbl(uint32_t nrml, uint32_t shft, uint32_t caps);
long mb_rom_cursconf(uint16_t rate, uint16_t attr);
long mb_rom_settime(uint32_t time);
long mb_rom_gettime(void);
long mb_rom_bioskeys(void);
long mb_rom_offgibit(uint16_t ormask);
long mb_rom_ongibit(uint16_t andmask);
long mb_rom_dosound(uint32_t ptr);
long mb_rom_kbdvbase(void);
long mb_rom_vsync(void);
long mb_rom_bconmap(uint16_t dev);
long mb_rom_vsetscreen(uint32_t lscrn, uint32_t pscrn, uint16_t rez, uint16_t mode);
long mb_rom_kbrate(uint16_t delay, uint16_t rate);

long mb_rom_gemdos_dispatch(uint16_t fnum, uint32_t *args);
long mb_rom_bios_dispatch(uint16_t fnum, uint32_t *args);
long mb_rom_xbios_dispatch(uint16_t fnum, uint32_t *args);

#endif /* MINTBOOT_MB_ROM_H */
