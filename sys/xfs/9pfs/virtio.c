/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * virtio-mmio transport for 9P
 *
 */

#include "mint/mint.h"
#include "mint/arch/asm_spl.h"
#include "libkern/libkern.h"

#include "9p.h"
#include "virtio.h"

static const struct virtio_info *ninep_vi;
static volatile u_int32_t       *ninep_base;
static struct virtio_vq         *ninep_vq;

static void *
aligned_kmalloc(ulong size, ulong align)
{
    ulong p = (ulong)kmalloc(size + align - 1);
    ulong offset = p % align;
    if (offset) {
        p += align - offset;
    }
    return (void *)p;
}

/*
 * In-flight transactions.
 * Each slot implicitly owns the corresponding group of
 * VIRTIO_DESC_PER_IO descriptors.
 *
 * The set of transactions must be <= VIRTIO_QUEUE_MAX
 * in order to ensure that ring slots aren't re-used
 * before they are reaped in virtio_interrupt_handler.
 */
static struct virtio_iovec *ninep_txn[VIRTIO_QUEUE_MAX];

/* allocate a transaction slot, waits forever */
static u_int16_t
virtio_alloc_txn(struct virtio_iovec *iov)
{
    short s = splhigh();
    for (;;)
    {
        for (int i =(long) 0; i < VIRTIO_QUEUE_MAX; i++) 
        {
            if (ninep_txn[i] == NULL) {
                ninep_txn[i] = iov;
                spl(s);
                return i;
            }
        }
        sleep(IO_Q, (long)&ninep_txn);
    }
}

/* handle an avail queue update interrupt */
static void
virtio_interrupt_handler(u_int32_t arg)
{
    static u_int16_t prev_uidx;
    const u_int16_t uidx = le2cpu16(ninep_vq->used.idx);

    /* reap new entries */
    while (prev_uidx != uidx) {
        KERNEL_TRACE("9pfs: virtio_interrupt_handler uidx %d", prev_uidx);
        /* get the descriptor for the completed entry and corresponding iov */
        u_int16_t desc = le2cpu32(ninep_vq->used.ring[prev_uidx].id);
        struct virtio_iovec *iov = ninep_txn[desc / VIRTIO_DESC_PER_IO];
        assert(iov);
        assert(iov->busy);

        /* get the transferred data length and update the lengths in the iov */
        u_int32_t len = le2cpu32(ninep_vq->used.ring[prev_uidx].len);
        if (len < iov->r_len)
        {
            /* truncate the R buffer */
            iov->r_len = len;
            len = 0;
        }
        else
        {
            /* adjust for R buffer */
            len -= iov->r_len;
        }
        assert(len <= iov->in_len);
        iov->in_len = len;

        /* complete the iov */
        iov->busy = false;
        wake(IO_Q, (long)iov);

        /* release the transaction */
        ninep_txn[desc] = NULL;
        wake(IO_Q, (long)&ninep_txn);

        /* move to the next queue entry */
        prev_uidx++;
    }
}

int
virtio_init(void)
{
    /* find the _VIO cookie */
    get_toscookie(VIRTIO_COOKIE, (ulong *)&ninep_vi);
    if (!ninep_vi)
    {
        KERNEL_ALERT("9pfs: no VirtIO cookie");
        return ENXIO;
    }
    if (ninep_vi->version != 0x56494f30) {
        KERNEL_ALERT("9pfs: unsupported structure version");
        return ENXIO;
    }

    volatile u_int32_t *base = (volatile u_int32_t *)ninep_vi->base_address;

    /* find the 9P transport */
    for (int i = 0; i < ninep_vi->num_devices; i++)
    {
        if (virtio_mmio_read32(base, VIRTIO_MMIO_DEVICE_ID) == VIRTIO_DEVICE_9P)
        {
            if (ninep_vi->acquire(i, virtio_interrupt_handler, 0) != VIRTIO_SUCCESS)
            {
                KERNEL_ALERT("9pfs: transport acquire failed");
                return EBUSY;
            }
            ninep_base = base;
            break;
        }
        base += (ninep_vi->device_size / sizeof(u_int32_t));
    }
    if (!ninep_base)
    {
        KERNEL_ALERT("9pfs: no transport");
        return ENXIO;
    }
    KERNEL_TRACE("9pfs: virtio-mmio @%p", ninep_base);

    /* tell the device a driver has appeared */
    u_int32_t status = VIRTIO_STAT_ACKNOWLEDGE | VIRTIO_STAT_DRIVER;
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_STATUS, status);

    /* negotiate features */
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_DRIVER_FEATURES, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_DRIVER_FEATURES, VIRTIO_FEATURE_VERSION_1);
    status |= VIRTIO_STAT_FEATURES_OK;
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_STATUS, status);
    if ((virtio_mmio_read32(ninep_base, VIRTIO_MMIO_STATUS) & VIRTIO_STAT_FEATURES_OK) == 0)
    {
        KERNEL_ALERT("9pfs: feature negotiation failed");
        return EIO;
    }

    /* acknowledge features */
    status |= VIRTIO_STAT_DRIVER_OK;
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_STATUS, status);

    /* alloc and zero the rings  */
    ninep_vq = aligned_kmalloc(sizeof(*ninep_vq), 16);

    /* tell the device where the descriptors and rings are */
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_SEL, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_MAX);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_DESC_LOW, (u_int32_t)&ninep_vq->desc[0]);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_DESC_HIGH, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_AVAIL_LOW, (u_int32_t)&ninep_vq->avail);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_AVAIL_HIGH, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_USED_LOW, (u_int32_t)&ninep_vq->used);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_USED_HIGH, 0);
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_READY, 1);

    KERNEL_DEBUG("9pfs: virtio setup done");
    return 0;
}

static void
virtio_submit(struct virtio_iovec *iov)
{
    /* get a transaction / descriptor set */
    const u_int16_t odesc = virtio_alloc_txn(iov);
    u_int16_t desc = odesc;
    iov->busy = true;

    /* put the T buffer in the first descriptor */
    KERNEL_TRACE("9pfs: t_buf -> %d/%p/%d", desc, iov->t_buf, iov->t_len);
    ninep_vq->desc[desc].addr  = cpu2le32((u_int32_t)iov->t_buf);
    ninep_vq->desc[desc].len   = cpu2le32(iov->t_len);
    ninep_vq->desc[desc].flags = cpu2le16(VIRTQ_DESC_F_NEXT);
    ninep_vq->desc[desc].next  = cpu2le16(desc + 1);
    desc++;

    /* if there's an out buffer, put it in the next descriptor */
    if (iov->out_len > 0)
    {
        KERNEL_TRACE("9pfs: out_buf -> %d/%p/%d", desc, iov->out_buf, iov->out_len);
        ninep_vq->desc[desc].addr  = cpu2le32((u_int32_t)iov->out_buf);
        ninep_vq->desc[desc].len   = cpu2le32(iov->out_len);
        ninep_vq->desc[desc].flags = cpu2le16(VIRTQ_DESC_F_NEXT);
        ninep_vq->desc[desc].next  = cpu2le16(desc + 1);
        desc++;
    }

    /* put the R buffer in the next descriptor */
    KERNEL_TRACE("9pfs: r_buf -> %d/%p/%d", desc, iov->r_buf, iov->r_len);
    ninep_vq->desc[desc].addr  = cpu2le32((u_int32_t)iov->r_buf);
    ninep_vq->desc[desc].len   = cpu2le32(iov->r_len);

    /* if there's an in buffer, put it in the next descriptor */
    if (iov->in_len > 0)
    {
        ninep_vq->desc[desc].flags = cpu2le16(VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT);
        ninep_vq->desc[desc].next  = cpu2le16(desc + 1);
        desc++;
        KERNEL_TRACE("9pfs: in_buf -> %d/%p/%d", desc, iov->in_buf, iov->in_len);
        ninep_vq->desc[desc].addr  = cpu2le32((u_int32_t)iov->in_buf);
        ninep_vq->desc[desc].len   = cpu2le32(iov->in_len);
    }
    ninep_vq->desc[desc].flags = cpu2le16(VIRTQ_DESC_F_WRITE);

    /* get the index of the next avail ring slot */
    const u_int16_t aidx = le2cpu16(ninep_vq->avail.idx);
    KERNEL_TRACE("9pfs: virtio_submit iovec %p desc %d avail %d", iov, odesc, aidx);

    /* post the descriptors to the ring */
    ninep_vq->avail.ring[aidx % VIRTIO_QUEUE_MAX] = cpu2le16(odesc);

    /* make the new transaction visible */
    __sync_synchronize();
    ninep_vq->avail.idx = cpu2le16(aidx + 1);

    /* notify the host */
    __sync_synchronize();
    virtio_mmio_write32(ninep_base, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

static void
virtio_wait_complete(struct virtio_iovec *iov)
{
    KERNEL_TRACE("9pfs: virtio_wait_complete %p", iov);
//    short s = splhigh();
    while (iov->busy)
    {
        sleep(IO_Q, (long)&iov);
    }
//    spl(s);
}

void
virtio_transact(struct virtio_iovec *iov)
{
    virtio_submit(iov);
    virtio_wait_complete(iov);
}
