#include "mintboot/mb_rom.h"
#include "mintboot/mb_bdos_state.h"
#include "mintboot/common/fs_fat/mb_fat_internal.h"
#include "mintboot/mb_errors.h"

long mb_bdos_dsetpath(const char *path)
{
	struct mb_fat_dirent ent;
	const char *p = path;
	char resolved[256];
	uint32_t out_idx = 0;
	const char *base;
	uint32_t i;
	uint16_t drive;

	if (!path || !path[0])
		return MB_ERR_PTHNF;

	if (p[1] == ':')
		return MB_ERR_PTHNF;

	drive = mb_bdos_get_current_drive();

	if (*p == '\\' || *p == '/')
		goto absolute_path;

	base = mb_bdos_get_current_path(drive);
	if (!base || !base[0])
		base = "\\";
	for (i = 0; base[i] && out_idx + 1 < sizeof(resolved); i++)
		resolved[out_idx++] = base[i];
	if (out_idx == 0 || resolved[out_idx - 1] != '\\')
		resolved[out_idx++] = '\\';
	while (*p && out_idx + 1 < sizeof(resolved)) {
		char c = *p++;
		if (c == '/')
			c = '\\';
		resolved[out_idx++] = c;
	}
	if (*p)
		return MB_ERR_PTHNF;
	resolved[out_idx] = '\0';
	p = resolved;
	goto lookup_path;

absolute_path:
	while (*p == '\\' || *p == '/')
		p++;
	resolved[out_idx++] = '\\';
	while (*p && out_idx + 1 < sizeof(resolved)) {
		char c = *p++;
		if (c == '/')
			c = '\\';
		resolved[out_idx++] = c;
	}
	if (*p)
		return MB_ERR_PTHNF;
	resolved[out_idx] = '\0';
	p = resolved;

lookup_path:
	if (p[1] == '\0')
		goto store_root;

	{
		int rc = mb_fat_find_path(drive, p, &ent);
		if (rc != 0)
			return MB_ERR_PTHNF;
	}

	if (drive >= MB_MAX_DRIVES)
		return MB_ERR_PTHNF;

	if (!(ent.attr & 0x10u))
		return MB_ERR_PTHNF;

store_root:
	if (p[1] == '\0') {
		if (mb_bdos_set_current_path(drive, "\\") != 0)
			return MB_ERR_PTHNF;
		return 0;
	}

	if (mb_bdos_set_current_path(drive, p) != 0)
		return MB_ERR_PTHNF;
	return 0;
}
