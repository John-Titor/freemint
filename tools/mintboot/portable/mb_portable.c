#include "mintboot/mb_portable.h"
#include "mintboot/mb_tests.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_cookie.h"
#include "mintboot/mb_lowmem.h"

extern void mb_install_vector_base(void);

struct mb_cookie_jar mb_cookie_jar;
char mb_cmdline[128];

#define MB_COOKIE_PTR_ADDR      0x000005a0u
#define MB_COOKIE_STORAGE_ADDR  0x00000600u
#define MB_COOKIE_CAPACITY      16u

static void mb_portable_init_cookies(void)
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

static void mb_etv_critic_stub(void)
{
}

static void mb_etv_term_stub(void)
{
}

static void mb_etv_timer_stub(void)
{
	(*mb_lm_vbclock())++;
}

static void mb_portable_init_lowmem(void)
{
	*mb_lm_etv_critic() = (uint32_t)(uintptr_t)mb_etv_critic_stub;
	*mb_lm_etv_term() = (uint32_t)(uintptr_t)mb_etv_term_stub;
	*mb_lm_etv_timer() = (uint32_t)(uintptr_t)mb_etv_timer_stub;
	*mb_lm_vbclock() = 0;
}

void mb_portable_setup_vectors(void)
{
	mb_install_vector_base();
}

void mb_portable_setup_traps(void)
{
	/* TODO: install ROM_* trap emulation handlers. */
}

void mb_portable_run_tests(void);

void mb_portable_boot(struct mb_boot_info *info)
{
	(void)info;

	mb_cmdline[0] = '\0';
	mb_portable_init_lowmem();
	mb_portable_init_cookies();
	mb_portable_setup_vectors();
	mb_portable_setup_traps();
	mb_portable_run_tests();
	mb_log_puts("mintboot: portable init complete\r\n");
	/* TODO: load/relocate kernel, finalize boot info, jump to entry. */
}
