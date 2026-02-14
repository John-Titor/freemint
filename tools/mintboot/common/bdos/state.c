#include "mintboot/mb_bdos_state.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"

#include <string.h>

#define MB_EMUTOS_DTA_SIZE 44
#define MB_GEMDOS_PATH_MAX 256

static uint8_t mb_default_dta[MB_EMUTOS_DTA_SIZE];
static void *mb_bdos_dta = mb_default_dta;
static uint16_t mb_current_drive;
static char mb_current_path[26][MB_GEMDOS_PATH_MAX];

static void mb_set_default_path(uint16_t drive)
{
	if (drive >= 26)
		return;
	if (mb_current_path[drive][0])
		return;
	mb_current_path[drive][0] = '\\';
	mb_current_path[drive][1] = '\0';
}

void mb_bdos_set_current_drive(uint16_t drive)
{
	mb_current_drive = drive;
	mb_set_default_path(drive);
}

uint16_t mb_bdos_get_current_drive(void)
{
	return mb_current_drive;
}

const char *mb_bdos_get_current_path(uint16_t drive)
{
	if (drive >= 26)
		return "\\";
	if (!mb_current_path[drive][0])
		return "\\";
	return mb_current_path[drive];
}

int mb_bdos_set_current_path(uint16_t drive, const char *path)
{
	size_t len;

	if (!path)
		return -1;
	if (drive >= 26)
		return -1;
	len = strlen(path);
	if (len + 1 > sizeof(mb_current_path[0]))
		return -1;
	memcpy(mb_current_path[drive], path, len + 1);
	return 0;
}

void mb_bdos_set_dta(void *dta)
{
	mb_bdos_dta = dta;
}

void *mb_bdos_get_dta(void)
{
	return mb_bdos_dta;
}
