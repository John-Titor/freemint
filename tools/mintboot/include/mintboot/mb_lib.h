#ifndef MINTBOOT_MB_LIB_H
#define MINTBOOT_MB_LIB_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int value, size_t len);
int memcmp(const void *a, const void *b, size_t len);
size_t strlen(const char *s);
void strlcpy(char *dst, const char *src, size_t n);
void strlcat(char *dst, const char *src, size_t n);

#endif
