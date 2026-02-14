#include "mintboot/mb_cookie.h"

struct mb_cookie_jar mb_cookie_jar;

#define MB_COOKIE_PTR_ADDR      0x000005a0u
#define MB_COOKIE_STORAGE_ADDR  0x00000600u
#define MB_COOKIE_CAPACITY      16u

static void mb_cookie_terminate(struct mb_cookie_jar *jar, size_t idx)
{
	jar->entries[idx].id = 0;
	jar->entries[idx].value = 0;
}

void mb_cookie_init(struct mb_cookie_jar *jar, struct mb_cookie *storage, size_t capacity)
{
	if (!jar || !storage || capacity == 0)
		return;

	jar->entries = storage;
	jar->capacity = capacity;
	mb_cookie_terminate(jar, 0);
}

int mb_cookie_set(struct mb_cookie_jar *jar, uint32_t id, uint32_t value)
{
	size_t i;

	if (!jar || !jar->entries || jar->capacity == 0 || id == 0)
		return -1;

	for (i = 0; i + 1 < jar->capacity; i++) {
		if (jar->entries[i].id == id || jar->entries[i].id == 0) {
			jar->entries[i].id = id;
			jar->entries[i].value = value;
			mb_cookie_terminate(jar, i + 1);
			return 0;
		}
	}

	return -1;
}

int mb_cookie_get(struct mb_cookie_jar *jar, uint32_t id, uint32_t *value)
{
	size_t i;

	if (!jar || !jar->entries || id == 0)
		return -1;

	for (i = 0; i < jar->capacity; i++) {
		if (jar->entries[i].id == 0)
			break;
		if (jar->entries[i].id == id) {
			if (value)
				*value = jar->entries[i].value;
			return 0;
		}
	}

	return -1;
}

void mb_cookie_init_defaults(void)
{
	struct mb_cookie *storage = (struct mb_cookie *)MB_COOKIE_STORAGE_ADDR;
	volatile struct mb_cookie **jar_ptr = (volatile struct mb_cookie **)MB_COOKIE_PTR_ADDR;

	mb_cookie_init(&mb_cookie_jar, storage, MB_COOKIE_CAPACITY);
	*jar_ptr = mb_cookie_entries(&mb_cookie_jar);

	/* Defaults; board code may override. */
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'C', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'F', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'M', 'C', 'H'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'V', 'D', 'O'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'S', 'N', 'D'), 0xffffffffu);
}

struct mb_cookie *mb_cookie_entries(struct mb_cookie_jar *jar)
{
	if (!jar)
		return NULL;
	return jar->entries;
}
