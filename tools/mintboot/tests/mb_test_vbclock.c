#include "mintboot/mb_portable.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_rom.h"
#include "mb_tests_internal.h"

void mb_tests_vbclock(void)
{
	volatile uint32_t *vbclock = mb_lm_vbclock();
	volatile uint32_t *frclock = mb_lm_frclock();
	volatile uint32_t *hz_200 = mb_lm_hz_200();
	uint32_t start_vb;
	uint32_t start_fr;
	uint32_t start_hz;
	uint32_t current_vb;
	uint32_t current_fr;
	uint32_t current_hz;
	uint32_t spins;
	uint32_t vb_delta;
	uint32_t fr_delta;
	uint32_t hz_delta;
	uint32_t expected_hz;
	uint32_t hz_diff;
	long start_rtc;
	long current_rtc = 0;

	start_vb = *vbclock;
	start_fr = *frclock;
	start_hz = *hz_200;
	start_rtc = mb_rom_dispatch.gettime();
	for (spins = 0; spins < 50000000u; spins++) {
		current_vb = *vbclock;
		current_fr = *frclock;
		current_hz = *hz_200;
		current_rtc = mb_rom_dispatch.gettime();
		vb_delta = current_vb - start_vb;
		fr_delta = current_fr - start_fr;
		hz_delta = current_hz - start_hz;
		if (vb_delta >= 25u &&
		    fr_delta >= 25u &&
		    hz_delta >= 100u) {
			expected_hz = fr_delta * 4u;
			hz_diff = (hz_delta > expected_hz)
				  ? (hz_delta - expected_hz)
				  : (expected_hz - hz_delta);
			if (hz_diff > 4u)
				mb_panic("HZ_200 mismatch fr=%u hz=%u diff=%u",
					 fr_delta, hz_delta, hz_diff);
			return;
		}
	}

	if (current_rtc == start_rtc)
		mb_panic("RTC did not advance start=%08x end=%08x",
			 (uint32_t)start_rtc, (uint32_t)current_rtc);
	mb_panic("timer counters did not update vb=%u->%u fr=%u->%u hz=%u->%u",
		 start_vb, current_vb, start_fr, current_fr, start_hz, current_hz);
}
