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

#include "mint/mint.h"
#include "mint/endian.h"

#include "libkern/libkern.h"

#include "9p.h"
#include "9p_proto.h"

bool
ninep_attach(void)
{
    struct virtio_iovec iov;
    struct ninep_small_buf tb, rb;

    iov.t_buf = &tb;
    iov.t_len = sizeof(tb);
    iov.r_buf = &rb;
    iov.out_len = 0;
    iov.in_len = 0;

    ninep_make_Tversion(&tb);
    iov.r_len = sizeof(rb);
    virtio_transact(&iov);
    if (!ninep_check_Rversion(&rb)) {
        KERNEL_ALERT("9pfs: protocol negotiation failed");
        return false;
    }
    KERNEL_DEBUG(("p9fs: version negotiation completed"));

    ninep_make_Tattach(&tb);
    iov.r_len = sizeof(rb);
    virtio_transact(&iov);
    if (!ninep_check_R(Tattach, &rb, NULL)) {
        KERNEL_DEBUG(("9pfs: attach failed"));
        return false;
    }

    return true;
}

ninep_errno_t
ninep_p_clunk(ninep_fid_t fid)
{
    struct ninep_small_buf tb, rb;
    struct virtio_iovec iov = {
        .t_buf = &tb,
        .t_len = sizeof(tb),
        .out_len = 0,
        .r_buf = &rb,
        .r_len = sizeof(rb),
        .in_len = 0
    };

    ninep_make_Tclunk(&tb, fid);
    virtio_transact(&iov);
    if (ninep_check_R(Tclunk, *rb)) {
        return 0;
    } else {
        ninep_errno_t err;
        if (ninep_check_Rerror(rb, &err)) {
            return err;
        }
    }
    return EIO;
}