#include "mintboot/mb_lib.h"

#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t len)
{
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	size_t i;

	for (i = 0; i < len; i++)
		d[i] = s[i];
	return dst;
}

void *memset(void *dst, int value, size_t len)
{
	uint8_t *d = (uint8_t *)dst;
	size_t i;

	for (i = 0; i < len; i++)
		d[i] = (uint8_t)value;
	return dst;
}

int memcmp(const void *a, const void *b, size_t len)
{
	const uint8_t *pa = (const uint8_t *)a;
	const uint8_t *pb = (const uint8_t *)b;
	size_t i;

	for (i = 0; i < len; i++) {
		if (pa[i] != pb[i])
			return (int)pa[i] - (int)pb[i];
	}
	return 0;
}

size_t strlen(const char *s)
{
	size_t len = 0;

	while (s[len])
		len++;
	return len;
}

void strlcpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;

	if (!n)
		return;

	for (i = 0; i + 1 < n && src[i]; i++)
		dst[i] = src[i];
	dst[i] = '\0';
}

void strlcat(char *dst, const char *src, size_t n)
{
	size_t dlen = 0;
	size_t i;

	if (!n)
		return;

	while (dlen < n && dst[dlen])
		dlen++;

	if (dlen == n)
		return;

	for (i = 0; dlen + i + 1 < n && src[i]; i++)
		dst[dlen + i] = src[i];
	dst[dlen + i] = '\0';
}
