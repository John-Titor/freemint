#include "mintboot/mb_rom.h"

long mb_xbios_keytbl(uint32_t nrml, uint32_t shft, uint32_t caps)
{
	struct mb_keytab {
		uint8_t *nrml;
		uint8_t *shft;
		uint8_t *caps;
	} __attribute__((packed));
	static uint8_t keytab_data[128];
	static struct mb_keytab keytab = {
		.nrml = keytab_data,
		.shft = keytab_data,
		.caps = keytab_data,
	};

	(void)nrml;
	(void)shft;
	(void)caps;

	return (long)(uintptr_t)&keytab;
}
