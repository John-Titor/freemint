/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _9p_h
# define _9p_h

#define NINEP_MSIZE     0x4000
#define NINEP_ROOT_FID  0xf000

/*
 * 9p_proto.h
 */

extern bool ninep_attach(void);


/*
 * virtio.c
 */

struct virtio_iovec
{
    volatile bool   busy;
    void            *t_buf;
    void            *out_buf;
    void            *r_buf;
    void            *in_buf;
    u_int16_t       t_len;
    u_int16_t       out_len;
    u_int16_t       r_len;
    u_int16_t       in_len;
};

extern int virtio_init(void);
extern void virtio_transact(struct virtio_iovec *iov);

# endif /* _9p_h */
