#include "mintboot/mb_rom.h"
#include "mintboot/mb_errors.h"
#include "mintboot/mb_portable.h"

long mb_bdos_mshrink(uint16_t zero, uint32_t base, uint32_t len)
{
	mb_log_printf("Mshrink(zero=%u, base=%08x, len=%u)\r\n",
		      (uint32_t)zero, base, len);
	if (zero != 0)
		return MB_ERR_INVFN;
	if (len == 0)
		return MB_ERR_INVFN;
	return 0;
}
