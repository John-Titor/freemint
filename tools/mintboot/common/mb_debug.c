#include "mintboot/mb_debug.h"
#include "mintboot/mb_common.h"

#include <stdint.h>
#include <stddef.h>

#define MB_DEBUG_VECTOR_COUNT 64u

static const char *mb_debug_last_entrypoint;
static uint16_t mb_debug_last_function;
static uint32_t mb_debug_last_return_address;
static long mb_debug_last_return_value;
static int mb_debug_last_return_valid;
static uint32_t mb_debug_handler_depth;

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
}

void mb_debug_bios_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
}

void mb_debug_xbios_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("xbios", fnum, retaddr);
}

void mb_debug_xbios_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
}

void mb_debug_bdos_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("bdos", fnum, retaddr);
}

void mb_debug_bdos_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
}

void mb_debug_aes_vdi_enter(uint16_t fnum, uint16_t *args, uint32_t *retaddr)
{
	(void)args;
	mb_debug_handler_enter("aes_vdi", fnum, retaddr);
}

void mb_debug_aes_vdi_exit(uint16_t fnum, uint16_t *args, uint32_t *retaddr, long ret)
{
	(void)fnum;
	(void)args;
	(void)retaddr;
	mb_debug_handler_exit(ret);
}

void mb_debug_timer_tick(void)
{
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
