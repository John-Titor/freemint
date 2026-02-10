/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _9p_h
# define _9p_h

/*
 * version
 */

# define VER_MAJOR  0
# define VER_MINOR  0

/*
 * configuration
 */

#define _drvbits        *(unsigned long *)0x4C2
#define DRIVE_NUM       ('V' - 'A')
#define DRIVE_BIT       (1UL << DRIVE_NUM)

#define NINEP_MAXIO     0x4000
#define NINEP_MSIZE     (NINEP_MAXIO + 23)
#define NINEP_ROOT_FID  0xf000
#define NINEP_MAXFILES  256

/*
 * debugging stuff
 */

# if 1
# define FS_DEBUG 1
# endif

# ifdef FS_DEBUG

# define FORCE(x)
# define ALERT(x)   KERNEL_ALERT x
# define DEBUG(x)   KERNEL_DEBUG x
# define TRACE(x)   KERNEL_TRACE x

# else

# define FORCE(x)
# define ALERT(x)   KERNEL_ALERT x
# define DEBUG(x)
# define TRACE(x)

# endif

/*
 * 9p_proto.h
 */

typedef u_int16_t   ninep_fid_t;
typedef u_int32_t   ninep_errno_t;

/* open mode */
typedef u_int8_t    ninep_mode_t;

#define NINEP_OREAD     0x00UL
#define NINEP_OWRITE    0x01UL
#define NINEP_ORDWR     0x02UL
#define NINEP_OEXEC     0x03UL
#define NINEP_OTRUNC    0x10UL

/* permission bits */
typedef u_int32_t   ninep_perm_t;

#define NINEP_DMSYMLINK 0x02000000UL
#define NINEP_DMEXCL    0x20000000UL
#define NINEP_DMAPPEND  0x40000000UL
#define NINEP_DMDIR     0x80000000UL

/* fid type bits */
#define NINEP_QTDIR     0x80
#define NINEP_QTAPPEND  0x40
#define NINEP_QTEXCL    0x20
#define NINEP_QTMOUNT   0x10
#define NINEP_QTLINK    0x02
#define NINEP_QTFILE    0x00

enum
{
    NINEP_FT_DIR,
    NINEP_FT_REG,
    NINEP_FT_LNK
};

extern bool ninep_p_attach(void);
extern ninep_errno_t ninep_p_walk(ninep_fid_t source_fid, ninep_fid_t *target_fid, const char *name);
extern ninep_errno_t ninep_p_clunk(ninep_fid_t fid);
extern ninep_errno_t ninep_p_readdir(ninep_fid_t fid, ushort index, char *name, short namelen);
extern ninep_errno_t ninep_p_remove(ninep_fid_t fid);
extern ninep_errno_t ninep_p_write(ninep_fid_t fid, long offset, void *buf, long count);

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
