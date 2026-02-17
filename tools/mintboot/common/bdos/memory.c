#include "mintboot/mb_bdos_mem.h"
#include "mintboot/mb_errors.h"

#define MB_MEM_BLOCK_MAX 8u

struct mb_mem_block {
	uint32_t base;
	uint32_t size;
	uint8_t pool;
	uint8_t is_free;
	uint8_t active;
};

static uint32_t mb_pool_base[2];
static uint32_t mb_pool_size[2];
static struct mb_mem_block mb_mem_blocks[MB_MEM_BLOCK_MAX];

static uint32_t mb_align4(uint32_t value)
{
	return (value + 3u) & ~3u;
}

static int mb_mem_alloc_slot(void)
{
	uint32_t i;

	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		if (!mb_mem_blocks[i].active)
			return (int)i;
	}
	return -1;
}

static void mb_mem_reset_pool(uint8_t pool)
{
	int slot;
	uint32_t i;

	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		if (mb_mem_blocks[i].active && mb_mem_blocks[i].pool == pool)
			mb_mem_blocks[i].active = 0;
	}

	if (mb_pool_size[pool] == 0)
		return;

	slot = mb_mem_alloc_slot();
	if (slot < 0)
		return;
	mb_mem_blocks[slot].base = mb_pool_base[pool];
	mb_mem_blocks[slot].size = mb_pool_size[pool];
	mb_mem_blocks[slot].pool = pool;
	mb_mem_blocks[slot].is_free = 1;
	mb_mem_blocks[slot].active = 1;
}

static void mb_mem_coalesce(uint8_t pool)
{
	uint32_t i;
	int merged;

	do {
		merged = 0;
		for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
			uint32_t j;

			if (!mb_mem_blocks[i].active || !mb_mem_blocks[i].is_free ||
			    mb_mem_blocks[i].pool != pool)
				continue;

			for (j = 0; j < MB_MEM_BLOCK_MAX; j++) {
				if (i == j)
					continue;
				if (!mb_mem_blocks[j].active || !mb_mem_blocks[j].is_free ||
				    mb_mem_blocks[j].pool != pool)
					continue;

				if (mb_mem_blocks[i].base + mb_mem_blocks[i].size ==
				    mb_mem_blocks[j].base) {
					mb_mem_blocks[i].size += mb_mem_blocks[j].size;
					mb_mem_blocks[j].active = 0;
					merged = 1;
					break;
				}
			}
			if (merged)
				break;
		}
	} while (merged);
}

static long mb_mem_largest_free(uint8_t pool)
{
	uint32_t i;
	uint32_t max = 0;

	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		if (!mb_mem_blocks[i].active || !mb_mem_blocks[i].is_free ||
		    mb_mem_blocks[i].pool != pool)
			continue;
		if (mb_mem_blocks[i].size > max)
			max = mb_mem_blocks[i].size;
	}
	return (long)max;
}

static long mb_mem_alloc_from_pool(uint8_t pool, uint32_t size)
{
	uint32_t i;

	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		int slot;
		uint32_t base;

		if (!mb_mem_blocks[i].active || !mb_mem_blocks[i].is_free ||
		    mb_mem_blocks[i].pool != pool)
			continue;
		if (mb_mem_blocks[i].size < size)
			continue;

		base = mb_mem_blocks[i].base;
		if (mb_mem_blocks[i].size == size) {
			mb_mem_blocks[i].is_free = 0;
			return (long)base;
		}

		slot = mb_mem_alloc_slot();
		if (slot < 0)
			return 0;

		mb_mem_blocks[slot].base = base;
		mb_mem_blocks[slot].size = size;
		mb_mem_blocks[slot].pool = pool;
		mb_mem_blocks[slot].is_free = 0;
		mb_mem_blocks[slot].active = 1;

		mb_mem_blocks[i].base += size;
		mb_mem_blocks[i].size -= size;
		return (long)base;
	}

	return 0;
}

static long mb_mem_alloc_mode(uint16_t mode, uint32_t size)
{
	switch (mode) {
	case 0u:
		return mb_mem_alloc_from_pool(MB_MEM_POOL_ST, size);
	case 1u:
		return mb_mem_alloc_from_pool(MB_MEM_POOL_TT, size);
	case 2u:
		if ((uint32_t)mb_mem_largest_free(MB_MEM_POOL_ST) >= size)
			return mb_mem_alloc_from_pool(MB_MEM_POOL_ST, size);
		return mb_mem_alloc_from_pool(MB_MEM_POOL_TT, size);
	case 3u:
		if ((uint32_t)mb_mem_largest_free(MB_MEM_POOL_TT) >= size)
			return mb_mem_alloc_from_pool(MB_MEM_POOL_TT, size);
		return mb_mem_alloc_from_pool(MB_MEM_POOL_ST, size);
	default:
		return MB_ERR_INVFN;
	}
}

static long mb_mem_largest_mode(uint16_t mode)
{
	long st = mb_mem_largest_free(MB_MEM_POOL_ST);
	long tt = mb_mem_largest_free(MB_MEM_POOL_TT);

	switch (mode) {
	case 0u:
		return st;
	case 1u:
		return tt;
	case 2u:
		return (st >= tt) ? st : tt;
	case 3u:
		return (tt >= st) ? tt : st;
	default:
		return MB_ERR_INVFN;
	}
}

static long mb_mem_free_ptr(uint32_t base)
{
	uint32_t i;

	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		uint8_t pool;

		if (!mb_mem_blocks[i].active || mb_mem_blocks[i].is_free)
			continue;
		if (mb_mem_blocks[i].base != base)
			continue;

		mb_mem_blocks[i].is_free = 1;
		pool = mb_mem_blocks[i].pool;
		mb_mem_coalesce(pool);
		return 0;
	}

	return MB_ERR_IHNDL;
}

static long mb_mem_shrink_ptr(uint16_t zero, uint32_t base, uint32_t len)
{
	uint32_t i;
	uint32_t new_len;

	if (zero != 0)
		return MB_ERR_INVFN;

	new_len = mb_align4(len);
	for (i = 0; i < MB_MEM_BLOCK_MAX; i++) {
		int slot;
		uint32_t tail_base;
		uint32_t tail_size;
		uint8_t pool;

		if (!mb_mem_blocks[i].active || mb_mem_blocks[i].is_free)
			continue;
		if (mb_mem_blocks[i].base != base)
			continue;

		if (new_len > mb_mem_blocks[i].size)
			return MB_ERR_RANGE;
		if (new_len == mb_mem_blocks[i].size)
			return 0;
		if (new_len == 0)
			return mb_mem_free_ptr(base);

		tail_base = mb_mem_blocks[i].base + new_len;
		tail_size = mb_mem_blocks[i].size - new_len;
		pool = mb_mem_blocks[i].pool;
		mb_mem_blocks[i].size = new_len;

		slot = mb_mem_alloc_slot();
		if (slot < 0)
			return MB_ERR_NHNDL;
		mb_mem_blocks[slot].base = tail_base;
		mb_mem_blocks[slot].size = tail_size;
		mb_mem_blocks[slot].pool = pool;
		mb_mem_blocks[slot].is_free = 1;
		mb_mem_blocks[slot].active = 1;
		mb_mem_coalesce(pool);
		return 0;
	}

	return MB_ERR_IHNDL;
}

void mb_bdos_mem_set_st_ram(uint32_t base, uint32_t size)
{
	mb_pool_base[MB_MEM_POOL_ST] = mb_align4(base);
	mb_pool_size[MB_MEM_POOL_ST] = (size > 3u) ? (size & ~3u) : 0u;
	mb_mem_reset_pool(MB_MEM_POOL_ST);
}

void mb_bdos_mem_set_tt_ram(uint32_t base, uint32_t size)
{
	mb_pool_base[MB_MEM_POOL_TT] = mb_align4(base);
	mb_pool_size[MB_MEM_POOL_TT] = (size > 3u) ? (size & ~3u) : 0u;
	mb_mem_reset_pool(MB_MEM_POOL_TT);
}

long mb_bdos_mem_malloc(int32_t amount)
{
	uint32_t size;

	if (amount < 0)
		return mb_mem_largest_free(MB_MEM_POOL_ST);
	if (amount == 0)
		return 0;

	size = mb_align4((uint32_t)amount);
	return mb_mem_alloc_from_pool(MB_MEM_POOL_ST, size);
}

long mb_bdos_mem_mxalloc(int32_t amount, uint16_t mode)
{
	uint32_t size;

	mode &= 0x3u;

	if (amount < 0)
		return mb_mem_largest_mode(mode);
	if (amount == 0)
		return 0;

	size = mb_align4((uint32_t)amount);
	return mb_mem_alloc_mode(mode, size);
}

long mb_bdos_mem_mfree(uint32_t base)
{
	return mb_mem_free_ptr(base);
}

long mb_bdos_mem_mshrink(uint16_t zero, uint32_t base, uint32_t len)
{
	return mb_mem_shrink_ptr(zero, base, len);
}
