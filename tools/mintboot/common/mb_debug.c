#include "mintboot/mb_debug.h"
#include "mintboot/mb_common.h"

#include <stdint.h>
#include <stddef.h>

#define MB_DEBUG_VECTOR_COUNT 64u

static uint32_t mb_debug_vectors[MB_DEBUG_VECTOR_COUNT];
static int mb_debug_vectors_ready;
static const char *mb_debug_last_entrypoint;
static uint16_t mb_debug_last_function;
static uint32_t mb_debug_last_return_address;
static long mb_debug_last_return_value;
static int mb_debug_last_return_valid;
static uint32_t mb_debug_handler_depth;

static void mb_debug_check_low_vectors(const char *source)
{
	volatile uint32_t *vectors = (volatile uint32_t *)0x0;
	uint32_t i;
	int changed = 0;

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
		changed = 1;
		mb_log_printf("debug: vec[%02u] changed (%s) 0x%08x -> 0x%08x\n",
			      i, source, was, now);
	}

	if (changed) {
		if (mb_debug_handler_depth)
			mb_panic("vector trample");
		mb_debug_dump_state();
	}
}

static uint32_t mb_debug_retaddr_value(uint32_t *retaddr)
{
	return (retaddr != NULL) ? *retaddr : 0u;
}

static void mb_debug_handler_enter(const char *entrypoint, uint16_t fnum,
				     uint32_t *retaddr)
{
	mb_debug_last_entrypoint = entrypoint;
	mb_debug_last_function = fnum;
	mb_debug_last_return_address = mb_debug_retaddr_value(retaddr);
	mb_debug_last_return_valid = 0;
	mb_debug_handler_depth++;
}

static void mb_debug_handler_exit(long ret)
{
	mb_debug_last_return_value = ret;
	mb_debug_last_return_valid = 1;
	if (mb_debug_handler_depth != 0u)
		mb_debug_handler_depth--;
}

void mb_debug_bios_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("bios", fnum, retaddr);
	mb_debug_check_low_vectors("bios-enter");
}

void mb_debug_bios_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
	mb_debug_check_low_vectors("bios-exit");
}

void mb_debug_xbios_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("xbios", fnum, retaddr);
	mb_debug_check_low_vectors("xbios-enter");
}

void mb_debug_xbios_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
	mb_debug_check_low_vectors("xbios-exit");
}

void mb_debug_bdos_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("bdos", fnum, retaddr);
	mb_debug_check_low_vectors("bdos-enter");
}

void mb_debug_bdos_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
	mb_debug_check_low_vectors("bdos-exit");
}

void mb_debug_vdi_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("vdi", fnum, retaddr);
	mb_debug_check_low_vectors("vdi-enter");
}

void mb_debug_vdi_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
	mb_debug_check_low_vectors("vdi-exit");
}

void mb_debug_timer_tick(void)
{
	mb_debug_check_low_vectors("timer");
}

void mb_debug_dump_state(void)
{
	const char *entry = mb_debug_last_entrypoint;

	if (entry == NULL)
		entry = "none";

	if (mb_debug_handler_depth) {
		mb_log_printf("debug: %sentry reason=%s fn=0x%04x retaddr=0x%08x\n",
		              mb_debug_handler_depth > 1 ? "nested " : "",
				      entry,
				      (uint32_t)mb_debug_last_function,
				      mb_debug_last_return_address);
	}
	if (mb_debug_last_return_valid) {
		mb_log_printf("debug: last return 0x%08x/%d\n",
			      (uint32_t)mb_debug_last_return_value,
			      (int)mb_debug_last_return_value);
	}
}
