#include "mintboot/mb_util.h"

#include <stddef.h>

#define MB_PATH_DUMP_MAX 64

const char *mb_guarded_str(const char *s)
{
	static char buf[MB_PATH_DUMP_MAX + 4];
	size_t i = 0;

	if (!s)
		return "(null)";

	for (; i < MB_PATH_DUMP_MAX; i++) {
		unsigned char c = (unsigned char)s[i];

		if (c == '\0') {
			buf[i] = '\0';
			return buf;
		}
		if (c < 0x20 || c > 0x7e)
			c = '.';
		buf[i] = (char)c;
	}

	buf[i++] = '.';
	buf[i++] = '.';
	buf[i++] = '.';
	buf[i] = '\0';
	return buf;
}
