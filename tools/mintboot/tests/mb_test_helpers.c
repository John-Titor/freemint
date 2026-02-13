#include "mintboot/mb_portable.h"
#include "mintboot/mb_tests.h"
#include "mb_tests_internal.h"

uint32_t mb_tests_strlen(const char *s)
{
	uint32_t len = 0;

	while (s[len])
		len++;
	return len;
}

void mb_tests_strcat(char *dst, uint32_t dstsz, const char *src)
{
	uint32_t dlen = mb_tests_strlen(dst);
	uint32_t i = 0;

	if (dlen >= dstsz)
		mb_panic("Test strcat overflow");

	while (src[i] && dlen + i + 1 < dstsz) {
		dst[dlen + i] = src[i];
		i++;
	}
	dst[dlen + i] = '\0';

	if (src[i])
		mb_panic("Test strcat overflow");
}
