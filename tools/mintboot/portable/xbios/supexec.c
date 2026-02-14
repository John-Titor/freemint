#include <stdint.h>

#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"

long mb_xbios_supexec(uint32_t func)
{
	long (*entry)(void) = (long (*)(void))(uintptr_t)func;
	static uint32_t calls;
	static long last_ret;
	static uint32_t stable_calls;
	long ret;

	if (entry == 0)
		return 0;

	ret = entry();
	calls++;
	if (ret == last_ret) {
		stable_calls++;
		if ((stable_calls % 200000u) == 0u) {
			mb_log_printf("Supexec stable ret=%08x stable=%u hz=%u fr=%u vb=%u\r\n",
				      (uint32_t)ret, stable_calls,
				      *mb_lm_hz_200(), *mb_lm_frclock(), *mb_lm_vbclock());
		}
	} else {
		last_ret = ret;
		stable_calls = 0;
		if ((calls % 20000u) == 0u) {
			mb_log_printf("Supexec ret=%08x hz=%u fr=%u vb=%u\r\n",
				      (uint32_t)ret,
				      *mb_lm_hz_200(), *mb_lm_frclock(), *mb_lm_vbclock());
		}
	}

	return ret;
}
