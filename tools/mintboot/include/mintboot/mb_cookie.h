#ifndef MINTBOOT_MB_COOKIE_H
#define MINTBOOT_MB_COOKIE_H

#include <stdint.h>
#include <stddef.h>

#define MB_COOKIE_ID(a, b, c, d) \
	((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))

int mb_cookie_set(uint32_t id, uint32_t value);
int mb_cookie_get(uint32_t id, uint32_t *value);
void mb_cookie_init_defaults(void);

#endif /* MINTBOOT_MB_COOKIE_H */
