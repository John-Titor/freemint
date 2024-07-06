/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _virtio_h
# define _virtio_h

#include "mint/endian.h"

/*
 * MMIO registers of interest for the 9P transport.
 */
#define VIRTIO_MMIO_DEVICE_ID           0x02
#define VIRTIO_MMIO_VENDOR_ID           0x03
#define VIRTIO_MMIO_DEVICE_FEATURES     0x04
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x05
#define VIRTIO_MMIO_DRIVER_FEATURES     0x08
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x09
#define VIRTIO_MMIO_QUEUE_SEL           0x0c
#define VIRTIO_MMIO_QUEUE_NUM_MAX       0x0d
#define VIRTIO_MMIO_QUEUE_NUM           0x0e
#define VIRTIO_MMIO_QUEUE_READY         0x11
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x14
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x18
#define VIRTIO_MMIO_STATUS              0x1c
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x20
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x21
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x24
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x25
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x28
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x29

#define VIRTIO_FEATURE_VERSION_1            (1<<0)
#define VIRTIO_MMIO_INT_VRING               (1<<0)
#define VIRTIO_STAT_ACKNOWLEDGE             (1<<0)
#define VIRTIO_STAT_DRIVER                  (1<<1)
#define VIRTIO_STAT_DRIVER_OK               (1<<2)
#define VIRTIO_STAT_FEATURES_OK             (1<<3)
#define VIRTIO_STAT_NEEDS_RESET             (1<<5)
#define VIRTIO_STAT_FAILED                  (1<<7)

static volatile inline u_int32_t virtio_mmio_read32(volatile u_int32_t *base, u_int16_t regidx)
{
    return le2cpu32(*(base + regidx));
}

static inline void virtio_mmio_write32(volatile u_int32_t *base, u_int16_t regidx, u_int32_t val)
{
    *(base + regidx) = cpu2le32(val);
}

/* max 8 I/Os at once */
#define VIRTIO_QUEUE_MAX    8

/* max 4 descriptors per I/O */
#define VIRTIO_DESC_PER_IO  4
#define VIRTIO_DESC_MAX     (VIRTIO_QUEUE_MAX * VIRTIO_DESC_PER_IO)

/* descriptor table, 2.7.5 */
struct virtq_desc
{
        u_int32_t addr;
        u_int32_t pad;
        u_int32_t len;
        u_int16_t flags;
#define VIRTQ_DESC_F_NEXT       1
#define VIRTQ_DESC_F_WRITE      2
        u_int16_t next;
} __attribute__((aligned(16)));

/* available ring, 2.7.6 */
struct virtq_avail
{
        u_int16_t flags;
        u_int16_t idx;
        u_int16_t ring[VIRTIO_QUEUE_MAX];
} __attribute__((aligned(2)));

/* used ring, 2.7.8 */
struct virtq_used_elem
{
        u_int32_t id;
        u_int32_t len;
};

struct virtq_used
{
        u_int16_t flags;
        u_int16_t idx;
        struct virtq_used_elem ring[VIRTIO_QUEUE_MAX];
} __attribute__((aligned(4)));

/* convenience wrapper; note member ordering facilitates alignment */
#define VIRTIO_VQ_ALIGN 16
struct virtio_vq
{
    struct virtq_desc   desc[VIRTIO_DESC_MAX];
    struct virtq_used   used;
    struct virtq_avail  avail;
};

/* VirtIO BIOS info / entrypoint structure */
struct virtio_info 
{
    u_int32_t version;
    u_int32_t base_address;
    u_int32_t device_size;
    u_int32_t num_devices;
    u_int32_t (* acquire)(u_int32_t handle, void (*interrupt_handler)(u_int32_t), u_int32_t arg);
    u_int32_t (* release)(u_int32_t handle);
};

#define VIRTIO_COOKIE       0x5f56494fUL
#define VIRTIO_DEVICE_9P    0x09

#define VIRTIO_SUCCESS          0UL
#define VIRTIO_ERROR            0xffffffffUL

# endif /* _virtio_h */
