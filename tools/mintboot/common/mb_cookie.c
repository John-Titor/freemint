#include "mintboot/mb_cookie.h"

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

struct mb_cookie *mb_cookie_entries(struct mb_cookie_jar *jar)
{
	if (!jar)
		return NULL;
	return jar->entries;
}
