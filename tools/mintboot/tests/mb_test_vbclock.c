#include "mintboot/mb_portable.h"
#include "mintboot/mb_lowmem.h"
#include "mb_tests_internal.h"

void mb_tests_vbclock(void)
{
	volatile uint32_t *vbclock = mb_lm_vbclock();
	volatile uint32_t *frclock = mb_lm_frclock();
	volatile uint32_t *hz_200 = mb_lm_hz_200();
	void (*handler)(void) = (void (*)(void))(uintptr_t)*mb_lm_etv_timer();
	uint32_t start_vb;
	uint32_t start_fr;
	uint32_t start_hz;
	uint32_t current_vb;
	uint32_t current_fr;
	uint32_t current_hz;
	uint32_t spins;

	start_vb = *vbclock;
	start_fr = *frclock;
	start_hz = *hz_200;
	for (spins = 0; spins < 5000000u; spins++) {
		if (handler)
			handler();
		current_vb = *vbclock;
		current_fr = *frclock;
		current_hz = *hz_200;
		if (current_vb != start_vb &&
		    current_fr > start_fr &&
		    current_hz >= start_hz + 4u) {
			if ((current_hz - start_hz) != (current_fr - start_fr) * 4u)
				mb_panic("HZ_200 mismatch fr=%u hz=%u",
					 current_fr - start_fr, current_hz - start_hz);
			return;
		}
	}

	mb_panic("timer counters did not update vb=%u->%u fr=%u->%u hz=%u->%u",
		 start_vb, current_vb, start_fr, current_fr, start_hz, current_hz);
}
