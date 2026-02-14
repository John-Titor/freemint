#ifndef MINTBOOT_MB_COOKIE_H
#define MINTBOOT_MB_COOKIE_H

#include <stdint.h>
#include <stddef.h>

struct mb_cookie {
	uint32_t id;
	uint32_t value;
};

struct mb_cookie_jar {
	struct mb_cookie *entries;
	size_t capacity; /* number of entries including terminator */
};

extern struct mb_cookie_jar mb_cookie_jar;

#define MB_COOKIE_ID(a, b, c, d) \
	((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))

void mb_cookie_init(struct mb_cookie_jar *jar, struct mb_cookie *storage, size_t capacity);
int mb_cookie_set(struct mb_cookie_jar *jar, uint32_t id, uint32_t value);
int mb_cookie_get(struct mb_cookie_jar *jar, uint32_t id, uint32_t *value);
void mb_cookie_init_defaults(void);

/* Returns pointer suitable for placing at 0x5a0. */
struct mb_cookie *mb_cookie_entries(struct mb_cookie_jar *jar);

#endif /* MINTBOOT_MB_COOKIE_H */
