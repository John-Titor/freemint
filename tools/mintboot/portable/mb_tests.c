#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"

void mb_portable_run_tests(void)
{
	char spec[] = "C:\\*.*";
	uint8_t dta[64];
	uint8_t buf[128];
	long rc;
	long fh;
	long n;

	mb_log_puts("mintboot: FAT test start\r\n");
	mb_rom_fsetdta(dta);
	rc = mb_rom_fsfirst(spec, 0x17);
	if (rc == 0) {
		for (;;) {
			rc = mb_rom_fsnext();
			if (rc != 0)
				break;
		}
	}

	fh = mb_rom_fopen("C:\\HELLO.TXT", 0);
	if (fh >= 0) {
		n = mb_rom_fread((uint16_t)fh, sizeof(buf),
				 (uint32_t)(uintptr_t)buf);
		mb_rom_fclose((uint16_t)fh);
		(void)n;
	}
	mb_log_puts("mintboot: FAT test done\r\n");
}
