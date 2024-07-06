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

#define NINEP_ROOT_FID      0xf000

#define _PREPARE_PACK(_b)   \
    u_int8_t    *p = _b + 4

#define _PACK(_f)                                           \
    *(typeof(_f) *)p = sizeof(_f) == 4 ? cpu2le32(_f) :     \
                        sizeof(_f) == 2 ? cpu2le16(_f) : _f;\
    p += sizeof(_f)

#define _PACK_TYPE(_t)  _PACK((u_int8_t)_t)
#define _PACK_TAG(_t)   _PACK((u_int16_t)_t)

#define _PACK_STR(_f)               \
    _PACK((u_int16_t)strlen(_f));   \
    memcpy(p, _f, strlen(_f));      \
    p += strlen(_f)

#define _FINALIZE(_b)   *(u_int32_t *)_b = cpu2le32(p - _b)

#define _PREPARE_CHECK(_b)                              \
    const u_int8_t *p = _b + 4;                         \
    const u_int8_t *e = _b + le2cpu32(*(u_int32_t *)_b)

#define _SKIP(_t)                       \
    if ((e - p) < sizeof(_t)            \
    {                                   \
        return false;                   \
    }                                   \
    p += sizeof(_t)

#define _SKIP_STR                       \
    if ((e - p) < 2)                    \
    {                                   \
        return false;                   \
    }                                   \
    p += 2 + cpu2le16(*(u_int16_t *)p)

#define _CHECK(_f)                                                  \
    if ((e - p) < sizeof(_f))                                       \
    {                                                               \
        return false;                                               \
    }                                                               \
    if (*(typeof(_f) *)p != (sizeof(_f) == 4 ? cpu2le32(_f) :       \
                             sizeof(_f) == 2 ? cpu2le16(_f) : _f))  \
    {                                                               \
        return false;                                               \
    }                                                               \
    p += sizeof(_f)

#define _EXTRACT(_f)                                            \
    if ((e - p) < sizeof(*_f))                                  \
    {                                                           \
        return false;                                           \
    }                                                           \
    if (_f != NULL)                                             \
    {                                                           \
        *_f = sizeof(*_f) == 4 ? cpu2le32(*(typeof(_f))p) :     \
              sizeof(*_f) == 2 ? cpu2le16(*(typeof(_f))p) : *p; \
    }                                                           \
    p += sizeof(*_f)

#define _EXTRACT_BUF(_f)            \
    if ((e - p) < sizeof(*_f))      \
    {                               \
        return false;               \
    }                               \
    if (_f != NULL)                 \
    {                               \
        memcpy(_f, p, sizeof(*_f)); \
    }                               \
    p += sizeof(_f)

#define _CHECK_STR(_f)              \
    _CHECK((u_int16_t)strlen(_f));  \
    if ((e - p) < strlen(_f))       \
    {                               \
        return false;               \
    }                               \
    if (memcmp(p, _f, strlen(_f)))  \
    {                               \
        return false;               \
    }                               \
    p += strlen(_f)

#define _CHECK_REPLY_TYPE(_t)   _CHECK((u_int8_t)(_t + 1))
#define _CHECK_TAG(_t)          _CHECK((u_int16_t)_t)

static void
ninep_make_Tversion(u_int8_t *buf)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Tversion);
    _PACK_TAG(0x0001);
    _PACK((u_int32_t)NINEP_MSIZE);
    _PACK_STR("9P2000.u");

    _FINALIZE(buf);

    KERNEL_TRACE("packed size %ld", p - buf);
    assert((p - buf) == 21);
}

static bool
ninep_check_Rversion(const u_int8_t *buf)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Tversion);
    _CHECK_TAG(0x0001);
    _CHECK((u_int32_t)NINEP_MSIZE);
    _CHECK_STR("9P2000.u");

    return true;
}

static void
ninep_make_Tattach(u_int8_t *buf)
{
    _PREPARE_PACK(buf);
    _PACK_TYPE(Tattach);
    _PACK_TAG(0x0001);
    _PACK((u_int32_t)NINEP_ROOT_FID);
    _PACK((u_int32_t)0);
    _PACK_STR("none");
    _PACK_STR("none");
}

static bool
ninep_check_Rattach(const u_int8_t *buf, struct ninep_qid *qid)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Tattach);
    _CHECK_TAG(0x0001);
    _EXTRACT_BUF(qid);

    return true;
}

bool
ninep_attach(void)
{
    struct virtio_iovec iov;
    u_int8_t tb[128];
    u_int8_t rb[128];

    iov.t_buf = &tb;
    iov.t_len = sizeof(tb);
    iov.r_buf = &rb;
    iov.out_len = 0;
    iov.in_len = 0;

    ninep_make_Tversion(tb);
    iov.r_len = sizeof(rb);
    virtio_transact(&iov);
    if (!ninep_check_Rversion(rb)) {
        KERNEL_ALERT("9pfs: protocol negotiation failed");
        return false;
    }
    KERNEL_DEBUG(("p9fs: version negotiation completed"));

    ninep_make_Tattach(tb);
    iov.r_len = sizeof(rb);
    virtio_transact(&iov);
    if (!ninep_check_Rattach(rb, NULL)) {
        KERNEL_DEBUG(("9pfs: attach failed"));
        return false;
    }

    return true;
}
