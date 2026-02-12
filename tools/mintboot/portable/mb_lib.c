#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int value, size_t len);
int memcmp(const void *a, const void *b, size_t len);
size_t strlen(const char *s);

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
