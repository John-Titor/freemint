#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mb_tests_internal.h"

void mb_tests_setexc(void)
{
	const uint16_t vnum = 200;
	const uint32_t new_vec = 0x12345678u;
	const uint32_t alt_vec = 0x87654321u;
	long prev;
	long cur;
	long rc;

	prev = (uint32_t)(uintptr_t)Setexc(vnum, 0xffffffffu);
	cur = (uint32_t)(uintptr_t)Setexc(vnum, new_vec);
	if (cur != prev)
		mb_panic("Setexc: prev mismatch %d vs %d", (int)cur, (int)prev);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, 0xffffffffu);
	if ((uint32_t)cur != new_vec)
		mb_panic("Setexc: query mismatch %d vs %u", (int)cur, new_vec);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, alt_vec);
	if ((uint32_t)cur != new_vec)
		mb_panic("Setexc: swap mismatch %d vs %u", (int)cur, new_vec);

	cur = (uint32_t)(uintptr_t)Setexc(vnum, prev);
	if ((uint32_t)cur != alt_vec)
		mb_panic("Setexc: restore mismatch %d vs %u", (int)cur, alt_vec);

	rc = (long)(uintptr_t)Setexc(256, 0xffffffffu);
	if (rc != 0)
		mb_panic("Setexc: vnum range rc=%d expected 0", (int)rc);
}
