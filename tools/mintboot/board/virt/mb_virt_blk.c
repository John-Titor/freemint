#include "mintboot/mb_errors.h"
#include "mintboot/mb_lib.h"
#include "mintboot/mb_rom.h"
#include "mb_virt_mmio.h"
#include "mb_virt_blk.h"

struct mb_virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

struct mb_virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[8];
	uint16_t used_event;
} __attribute__((packed));

struct mb_virtq_used_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

struct mb_virtq_used {
	uint16_t flags;
	uint16_t idx;
	struct mb_virtq_used_elem ring[8];
	uint16_t avail_event;
} __attribute__((packed));

struct mb_virtio_blk_req {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
} __attribute__((packed));

static struct mb_virtq_desc mb_blk_desc[8] __attribute__((aligned(16)));
static struct mb_virtq_avail mb_blk_avail __attribute__((aligned(2)));
static struct mb_virtq_used mb_blk_used __attribute__((aligned(4)));
static struct mb_virtio_blk_req mb_blk_req __attribute__((aligned(16)));
static volatile uint8_t mb_blk_status __attribute__((aligned(16)));
static uint16_t mb_blk_avail_idx;
static uint16_t mb_blk_queue_num;
static uint64_t mb_blk_capacity;
static int mb_blk_ready;
static int mb_blk_mmio_swap;
static uintptr_t mb_blk_mmio_base = VIRT_VIRTIO_MMIO_BASE;

static uint16_t mb_swap16(uint16_t v)
{
	return (uint16_t)((v >> 8) | (v << 8));
}

static uint32_t mb_swap32(uint32_t v)
{
	return ((v >> 24) & 0x000000ffu) |
	       ((v >> 8) & 0x0000ff00u) |
	       ((v << 8) & 0x00ff0000u) |
	       ((v << 24) & 0xff000000u);
}

static uint64_t mb_swap64(uint64_t v)
{
	return ((uint64_t)mb_swap32((uint32_t)(v >> 32))) |
	       ((uint64_t)mb_swap32((uint32_t)v) << 32);
}

static uint16_t mb_cpu_to_le16(uint16_t v)
{
	return mb_swap16(v);
}

static uint32_t mb_cpu_to_le32(uint32_t v)
{
	return mb_swap32(v);
}

static uint64_t mb_cpu_to_le64(uint64_t v)
{
	return mb_swap64(v);
}

static uint32_t mb_virtio_reg_read32(uint32_t reg)
{
	uint32_t v = mb_mmio_read32(mb_blk_mmio_base + reg);

	if (mb_blk_mmio_swap)
		v = mb_swap32(v);
	return v;
}

static void mb_virtio_reg_write32(uint32_t reg, uint32_t value)
{
	if (mb_blk_mmio_swap)
		value = mb_swap32(value);
	mb_mmio_write32(mb_blk_mmio_base + reg, value);
}

static int mb_virt_blk_wait_complete(void)
{
	uint32_t i;

	for (i = 0; i < 10000000u; i++) {
		if (mb_blk_status != 0xffu)
			return 0;
	}

	return -1;
}

static int mb_virt_blk_submit(uint32_t type, void *buf, uint32_t len, uint64_t sector)
{
	uint16_t slot;
	uint32_t isr;

	mb_blk_req.type = mb_cpu_to_le32(type);
	mb_blk_req.reserved = 0;
	mb_blk_req.sector = mb_cpu_to_le64(sector);
	mb_blk_status = 0xff;

	mb_blk_desc[0].addr = mb_cpu_to_le64((uint64_t)(uintptr_t)&mb_blk_req);
	mb_blk_desc[0].len = mb_cpu_to_le32(sizeof(mb_blk_req));
	mb_blk_desc[0].flags = mb_cpu_to_le16(VIRTQ_DESC_F_NEXT);
	mb_blk_desc[0].next = mb_cpu_to_le16(1);

	mb_blk_desc[1].addr = mb_cpu_to_le64((uint64_t)(uintptr_t)buf);
	mb_blk_desc[1].len = mb_cpu_to_le32(len);
	mb_blk_desc[1].flags = mb_cpu_to_le16(VIRTQ_DESC_F_NEXT |
					      ((type == VIRTIO_BLK_T_IN) ? VIRTQ_DESC_F_WRITE : 0u));
	mb_blk_desc[1].next = mb_cpu_to_le16(2);

	mb_blk_desc[2].addr = mb_cpu_to_le64((uint64_t)(uintptr_t)&mb_blk_status);
	mb_blk_desc[2].len = mb_cpu_to_le32(1);
	mb_blk_desc[2].flags = mb_cpu_to_le16(VIRTQ_DESC_F_WRITE);
	mb_blk_desc[2].next = 0;

	slot = (uint16_t)(mb_blk_avail_idx % mb_blk_queue_num);
	mb_blk_avail.ring[slot] = mb_cpu_to_le16(0);
	mb_blk_avail_idx++;
	mb_blk_avail.idx = mb_cpu_to_le16(mb_blk_avail_idx);

	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
	if (mb_virt_blk_wait_complete() != 0)
		return -1;

	isr = mb_virtio_reg_read32(VIRTIO_MMIO_INTERRUPT_STATUS);
	if (isr)
		mb_virtio_reg_write32(VIRTIO_MMIO_INTERRUPT_ACK, isr);

	if (mb_blk_status != 0)
		return -1;
	return 0;
}

static int mb_virt_blk_init(void)
{
	uint32_t magic;
	uint32_t version;
	uint32_t device_id;
	uint32_t status;
	uint32_t qmax;
	uint32_t cap_low;
	uint32_t cap_high;
	uint64_t capacity;
	uintptr_t slot_base = VIRT_VIRTIO_MMIO_BASE;
	uint32_t slot;
	uintptr_t desc_addr;
	uintptr_t avail_addr;
	uintptr_t used_addr;

	if (mb_blk_ready)
		return 0;

	for (slot = 0; slot < VIRT_VIRTIO_MMIO_SLOTS; slot++) {
		slot_base = VIRT_VIRTIO_MMIO_BASE + slot * VIRT_VIRTIO_MMIO_STRIDE;
		magic = mb_mmio_read32(slot_base + VIRTIO_MMIO_MAGIC_VALUE);
		if (magic == 0x74726976u)
			mb_blk_mmio_swap = 0;
		else if (magic == 0x76697274u)
			mb_blk_mmio_swap = 1;
		else
			continue;

		mb_blk_mmio_base = slot_base;
		magic = mb_virtio_reg_read32(VIRTIO_MMIO_MAGIC_VALUE);
		version = mb_virtio_reg_read32(VIRTIO_MMIO_VERSION);
		device_id = mb_virtio_reg_read32(VIRTIO_MMIO_DEVICE_ID);
		if (magic == 0x74726976u && version == 2u &&
		    device_id == VIRTIO_ID_BLOCK)
			break;
	}
	if (slot == VIRT_VIRTIO_MMIO_SLOTS)
		return -1;

	mb_virtio_reg_write32(VIRTIO_MMIO_STATUS, 0);
	status = VIRTIO_STATUS_ACKNOWLEDGE;
	mb_virtio_reg_write32(VIRTIO_MMIO_STATUS, status);
	status |= VIRTIO_STATUS_DRIVER;
	mb_virtio_reg_write32(VIRTIO_MMIO_STATUS, status);

	mb_virtio_reg_write32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
	mb_virtio_reg_write32(VIRTIO_MMIO_DRIVER_FEATURES, 0);
	mb_virtio_reg_write32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
	mb_virtio_reg_write32(VIRTIO_MMIO_DRIVER_FEATURES, 1u); /* VIRTIO_F_VERSION_1 */

	status |= VIRTIO_STATUS_FEATURES_OK;
	mb_virtio_reg_write32(VIRTIO_MMIO_STATUS, status);
	if ((mb_virtio_reg_read32(VIRTIO_MMIO_STATUS) &
	     VIRTIO_STATUS_FEATURES_OK) == 0)
		return -1;

	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_SEL, 0);
	qmax = mb_virtio_reg_read32(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (qmax < 3u)
		return -1;
	mb_blk_queue_num = (uint16_t)((qmax < 8u) ? qmax : 8u);
	memset(mb_blk_desc, 0, sizeof(mb_blk_desc));
	memset(&mb_blk_avail, 0, sizeof(mb_blk_avail));
	memset(&mb_blk_used, 0, sizeof(mb_blk_used));
	mb_blk_avail_idx = 0;

	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_NUM, mb_blk_queue_num);

	desc_addr = (uintptr_t)mb_blk_desc;
	avail_addr = (uintptr_t)&mb_blk_avail;
	used_addr = (uintptr_t)&mb_blk_used;
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t)desc_addr);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_DESC_HIGH, 0);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_AVAIL_LOW, (uint32_t)avail_addr);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_AVAIL_HIGH, 0);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_USED_LOW, (uint32_t)used_addr);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_USED_HIGH, 0);
	mb_virtio_reg_write32(VIRTIO_MMIO_QUEUE_READY, 1);

	status |= VIRTIO_STATUS_DRIVER_OK;
	mb_virtio_reg_write32(VIRTIO_MMIO_STATUS, status);

	cap_low = mb_virtio_reg_read32(VIRTIO_MMIO_CONFIG + 0);
	cap_high = mb_virtio_reg_read32(VIRTIO_MMIO_CONFIG + 4);
	capacity = ((uint64_t)cap_high << 32) | cap_low;
	if (capacity == 0)
		return -1;

	mb_blk_capacity = capacity;
	mb_blk_ready = 1;
	return 0;
}

long mb_virt_blk_rwabs(uint16_t rwflag, void *buf, uint16_t count,
		       uint16_t recno, uint16_t dev)
{
	uint64_t start;
	uint64_t end;
	uint32_t len;
	uint32_t type;

	if (mb_virt_blk_init() != 0)
		return -1;

	if (dev >= MB_MAX_DRIVES)
		return MB_ERR_DRIVE;

	if (dev != 2)
		return MB_ERR_DRIVE;

	start = recno;
	end = start + count;
	if (end > mb_blk_capacity)
		return -1;

	len = (uint32_t)count * 512u;
	type = ((rwflag & 1u) == 0) ? VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT;
	if (mb_virt_blk_submit(type, buf, len, start) != 0)
		return -1;

	return 0;
}
