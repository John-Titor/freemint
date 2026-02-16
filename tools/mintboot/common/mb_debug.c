#include "mintboot/mb_debug.h"
#include "mintboot/mb_common.h"

#include <stdint.h>
#include <stddef.h>

#define MB_DEBUG_VECTOR_COUNT 64u

static uint32_t mb_debug_vectors[MB_DEBUG_VECTOR_COUNT];
static int mb_debug_vectors_ready;

static void mb_debug_check_low_vectors(const char *source)
{
	volatile uint32_t *vectors = (volatile uint32_t *)(uintptr_t)mb_common_vector_base();
	uint32_t i;

	if (!mb_debug_vectors_ready) {
		for (i = 0; i < MB_DEBUG_VECTOR_COUNT; i++)
			mb_debug_vectors[i] = vectors[i];
		mb_debug_vectors_ready = 1;
		return;
	}

	for (i = 0; i < MB_DEBUG_VECTOR_COUNT; i++) {
		uint32_t now = vectors[i];
		uint32_t was = mb_debug_vectors[i];

		if (now == was)
			continue;

		mb_debug_vectors[i] = now;
		mb_log_printf("debug: vec[%02u] changed (%s) 0x%08x -> 0x%08x\n",
			      i, source, was, now);
	}
}

void mb_debug_bios_enter(uint16_t fnum, uint16_t *args)
{
	(void)fnum;
	(void)args;
	mb_debug_check_low_vectors("bios-enter");
}

void mb_debug_bios_exit(uint16_t fnum, uint16_t *args, long ret)
{
	(void)fnum;
	(void)args;
	(void)ret;
	mb_debug_check_low_vectors("bios-exit");
}

void mb_debug_xbios_enter(uint16_t fnum, uint16_t *args)
{
	(void)fnum;
	(void)args;
	mb_debug_check_low_vectors("xbios-enter");
}

void mb_debug_xbios_exit(uint16_t fnum, uint16_t *args, long ret)
{
	(void)fnum;
	(void)args;
	(void)ret;
	mb_debug_check_low_vectors("xbios-exit");
}

void mb_debug_bdos_enter(uint16_t fnum, uint16_t *args)
{
	(void)fnum;
	(void)args;
	mb_debug_check_low_vectors("bdos-enter");
}

void mb_debug_bdos_exit(uint16_t fnum, uint16_t *args, long ret)
{
	(void)fnum;
	(void)args;
	(void)ret;
	mb_debug_check_low_vectors("bdos-exit");
}

void mb_debug_vdi_enter(uint16_t fnum, uint16_t *args)
{
	(void)fnum;
	(void)args;
	mb_debug_check_low_vectors("vdi-enter");
}

void mb_debug_vdi_exit(uint16_t fnum, uint16_t *args, long ret)
{
	(void)fnum;
	(void)args;
	(void)ret;
	mb_debug_check_low_vectors("vdi-exit");
}

void mb_debug_timer_tick(void)
{
	mb_debug_check_low_vectors("timer");
}
