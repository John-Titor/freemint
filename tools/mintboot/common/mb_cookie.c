#include "mintboot/mb_cookie.h"
#include "mintboot/mb_lowmem.h"

#define MB_COOKIE_CAPACITY      20u

struct mb_cookie {
	uint32_t id;
	uint32_t value;
};

static struct mb_cookie mb_cookie_jar[MB_COOKIE_CAPACITY];

static struct mb_cookie *mb_get_cookie_jar(void)
{
	return (struct mb_cookie *)*mb_lm_p_cookies();
}

int mb_cookie_set(uint32_t id, uint32_t value)
{
	struct mb_cookie *cp = mb_get_cookie_jar();

	if (!cp)
		return -1;

	while (cp->id != 0) {
		if (cp->id == id) {
			cp->value = value;
			return 0;
		}
		cp++;
	}
	if (cp->value < 1) {
		return -1;
	}
	cp[1].id = 0;
	cp[1].value = cp->value - 1;
	cp->id = id;
	cp->value = value;
	return 0;
}

int mb_cookie_get(uint32_t id, uint32_t *value)
{
	const struct mb_cookie *cp = mb_get_cookie_jar();

	if (!cp)
		return -1;

	while (cp->id != 0) {
		if (cp->id == id) {
			if (value)
				*value = cp->value;
			return 0;
		}
		cp++;
	}
	return -1;
}

void mb_cookie_init_defaults(void)
{
	/* jar init */
	mb_cookie_jar[0].id = 0u;
	mb_cookie_jar[0].value = MB_COOKIE_CAPACITY - 1;
	*(struct mb_cookie **)mb_lm_p_cookies() = &mb_cookie_jar[0];

	/* Defaults; board code may override. */
	mb_cookie_set(MB_COOKIE_ID('_', 'C', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(MB_COOKIE_ID('_', 'F', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(MB_COOKIE_ID('_', 'M', 'C', 'H'), 0xffffffffu);
	mb_cookie_set(MB_COOKIE_ID('_', 'V', 'D', 'O'), 0xffffffffu);
	mb_cookie_set(MB_COOKIE_ID('_', 'S', 'N', 'D'), 0xffffffffu);
}
