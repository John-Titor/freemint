#include "mintboot/mb_rom.h"
#include "mintboot/mb_fat.h"

long mb_bdos_dcreate(const char *path)
{
	return mb_fat_dcreate(path);
}
