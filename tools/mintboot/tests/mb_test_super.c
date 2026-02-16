	#include <stdint.h>

#include "mintboot/mb_common.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_osbind.h"
#include "mb_tests_internal.h"

static uint32_t mb_tests_get_sp(void)
{
	uint32_t sp;

	__asm__ volatile("move.l %%sp,%0" : "=r"(sp));
	return sp;
}

void mb_tests_super(void)
{
	long rc;
	long old_ssp_1;
	long old_ssp_2;
	uint32_t user_sp_0;
	uint32_t user_sp_1;
	uint32_t super_sp;

	rc = Super(1L);
	if (rc != 0)
		mb_panic("Super test: Super(1) user rc=%d", (int)rc);

	user_sp_0 = mb_tests_get_sp();

	old_ssp_1 = Super(0L);
	super_sp = mb_tests_get_sp();

	if (old_ssp_1 <= 0)
		mb_panic("Super test: Super(0) old SSP=%d", (int)old_ssp_1);
	if (super_sp != user_sp_0)
		mb_panic("Super test: SSP!=old USP ssp=%08x usp0=%08x",
			 super_sp, user_sp_0);

	rc = Super(1L);
	if (rc == 0)
		mb_panic("Super test: Super(1) super rc=%d", (int)rc);

	(void)Super(old_ssp_1);
	user_sp_1 = mb_tests_get_sp();
	if (user_sp_1 != user_sp_0)
		mb_panic("Super test: user SP restore sp1=%08x sp0=%08x",
			 user_sp_1, user_sp_0);

	old_ssp_2 = Super(0L);
	if (old_ssp_2 != old_ssp_1)
		mb_panic("Super test: old SSP mismatch ssp2=%08x ssp1=%08x",
			 (uint32_t)old_ssp_2, (uint32_t)old_ssp_1);

	(void)Super(old_ssp_2);
	mb_log_puts("Super test: pass\n");
}
