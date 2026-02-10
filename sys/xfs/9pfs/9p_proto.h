/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _9p_proto_h
# define _9p_proto_h

/* T (transaction) types */
typedef enum
{
    Tversion    = 100,
    Tauth       = 102,
    Tattach     = 104,
    Terror      = 106,  /* not really a T */
    Tflush      = 108,
    Twalk       = 110,
    Topen       = 112,
    Tcreate     = 114,
    Tread       = 116,
    Twrite      = 118,
    Tclunk      = 120,
    Tremove     = 122,
    Tstat       = 124,
    Twstat      = 126
} ninep_T_t;

struct ninep_hdr
{
    u_int32_t   len;
    u_int8_t    type;
    u_int16_t   tag;
} __attribute__((packed));

struct ninep_qid
{
    u_int8_t    type;
    u_int32_t   version;
    u_int8_t    path[8];
} __attribute__((packed));

struct ninep_small_buf
{
    union
    {
        struct ninep_hdr    hdr;
        u_int8_t            buf[32];
    };
};

/* for Tcreate, Rstat, Twstat */
struct ninep_large_buf
{
    union
    {
        struct ninep_hdr    hdr;
        u_int8_t            buf[512];
    };
};

#define ninep_buf_len(_b)   le2cpu32(_b.hdr.len)

enum
{
    QTDIR       = 0x80,
    QTAPPEND    = 0x40,
    QTEXCL      = 0x20,
    QTMOUNT     = 0x10,
    QTAUTH      = 0x08,
    QTTMP       = 0x04,
    QTSYMLINK   = 0x02,
    QTLINK      = 0x01,
    QTFILE      = 0x00,
};

#define _PREPARE_PACK(_b)       \
    u_int8_t    *p = &_b->buf[4]

#define _PACK(_f)                                           \
    *(typeof(_f) *)p = sizeof(_f) == 8 ? cpu2le64(_f) :     \
                       sizeof(_f) == 4 ? cpu2le32(_f) :     \
                       sizeof(_f) == 2 ? cpu2le16(_f) : _f; \
    p += sizeof(_f)

#define _PACK_TYPE(_t)  _PACK((u_int8_t)_t)
#define _PACK_TAG(_t)   _PACK((u_int16_t)_t)

#define _PACK_STR(_f)               \
    _PACK((u_int16_t)strlen(_f));   \
    memcpy(p, _f, strlen(_f));      \
    p += strlen(_f)

#define _FINALIZE(_b)   _b->hdr.len = cpu2le32(p - &_b->buf[0])


#define _PREPARE_CHECK(_b)                              \
    const u_int8_t *p = &_b->buf[4];                    \
    const u_int8_t *e = &_b->buf[le2cpu32(_b->hdr.len)]

#define _SKIP(_t)                       \
    if ((e - p) < sizeof(_t))           \
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
    if (*(typeof(_f) *)p != (sizeof(_f) == 8 ? cpu2le64(_f) :       \
                             sizeof(_f) == 4 ? cpu2le32(_f) :       \
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
        *_f = sizeof(*_f) == 8 ? cpu2le64(*(typeof(_f))p) :     \
              sizeof(*_f) == 4 ? cpu2le32(*(typeof(_f))p) :     \
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
    p += sizeof(*_f)

#define _CHECK_STR(_f)              \
    do                              \
    {                               \
        uint16_t l = strlen(_f);    \
        _CHECK(l);                  \
        if ((e - p) < l)            \
        {                           \
            return false;           \
        }                           \
        if (memcmp(p, _f, l))       \
        {                           \
            return false;           \
        }                           \
        p += l;                     \
    } while(0)

#define _EXTRACT_STR(_s, _f)                                    \
    do                                                          \
    {                                                           \
        u_int16_t l;                                            \
        _EXTRACT(&l);                                           \
        u_int16_t least = (l < (_s - 1)) ? l : (_s - 1);        \
        memcpy(_f, p, least);                                   \
        _f[least] = '\0';                                       \
        p += least;                                             \
    } while(0)


#define _CHECK_REPLY_TYPE(_t)   _CHECK((u_int8_t)(_t + 1))
#define _CHECK_TAG(_t)          _CHECK((u_int16_t)_t)

static inline void
ninep_make_Tversion(struct ninep_small_buf *buf)
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

static inline bool
ninep_check_Rversion(const struct ninep_small_buf *buf)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Tversion);
    _CHECK_TAG(0x0001);
    _CHECK((u_int32_t)NINEP_MSIZE);
    _CHECK_STR("9P2000.u");

    return true;
}

static inline bool
ninep_check_Rerror(const struct ninep_small_buf *buf, 
                   ninep_errno_t *error)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Terror);
    _SKIP_STR;
    _EXTRACT(error);

    return true;
}

static inline void
ninep_make_Tattach(struct ninep_small_buf *buf)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Tattach);
    _PACK_TAG(0x0001);
    _PACK((u_int32_t)NINEP_ROOT_FID);
    _PACK((u_int32_t)0);
    _PACK_STR("none");
    _PACK_STR("none");
    _PACK(~0UL);

    _FINALIZE(buf);
}

static inline void
ninep_make_Twalk(struct ninep_large_buf *buf,
                 ninep_fid_t fid,
                 ninep_fid_t newfid,
                 const char *wname)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Twalk);
    _PACK_TAG(0x0001);
    _PACK(fid);
    _PACK(newfid);
    _PACK((u_int16_t)1);
    _PACK_STR(wname);

    _FINALIZE(buf);
}

static inline void
ninep_make_Topen(struct ninep_small_buf *buf,
                 ninep_fid_t fid,
                 ninep_mode mode)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Topen);
    _PACK_TAG(0x0001);
    _PACK(fid);
    _PACK(mode);

    _FINALIZE(buf);
}

static inline void
ninep_make_Tcreate(struct ninep_large_buf *buf,
                   ninep_fid_t fid,
                   const char *name,
                   ninep_perm_t perm,
                   ninep_mode_t mode,
                   const char *extension)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Tcreate);
    _PACK_TAG(0x0001);
    _PACK(fid);
    _PACK_STR(name);
    _PACK(perm);
    _PACK(mode);
    if (extension)
    {
        _PACK_STR(extension);
    }
    else
    {
        _PACK_STR("");
    }

    _FINALIZE(buf);
}

static inline void
ninep_make_Tread(struct ninep_small_buf *buf,
                 ninep_fid_t fid,
                 u_int64_t offset,
                 u_int32_t count)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Tread);
    _PACK_TAG(0x0001);
    _PACK(offset);
    _PACK(count);

    _FINALIZE(buf);
}

static inline bool
ninep_check_Rread(const struct ninep_small_buf *buf,
                  u_int32_t count)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Tread);
    _CHECK_TAG(0x0001);
    _CHECK_(count);

    return true;
}

static inline void
ninep_make_Twrite(struct ninep_small_buf *buf,
                 ninep_fid_t fid,
                 u_int64_t offset,
                 u_int32_t count)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(Twrite);
    _PACK_TAG(0x0001);
    _PACK(offset);
    _PACK(count);

    _FINALIZE(buf);
}

static inline bool
ninep_check_Rwrite(const struct ninep_small_buf *buf,
                  u_int32_t count)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Twrite);
    _CHECK_TAG(0x0001);
    _CHECK_(count);

    return true;
}

static inline bool
ninep_check_Rstat(const struct ninep_large_buf *buf,
                  ninep_perm_t *perm,
                  u_int32_t *atime,
                  u_int32_t *mtime,
                  llong *size,
                  char *extension,
                  ulong *uid,
                  ulong *gid)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Tstat);
    _CHECK_TAG(0x0001);
    _SKIP(u_int16_t);           /* struct size */
    _SKIP(u_int16_t);           /* type */
    _SKIP(u_int32_t);           /* dev */
    _SKIP(u_int8_t);            /* qid.type */
    _SKIP(u_int32_t);           /* qid.version */
    _SKIP(u_int64_t);           /* qid.path */
    _EXTRACT(perm);
    _EXTRACT(atime);
    _EXTRACT(mtime);
    _EXTRACT(size);
    _SKIP_STR;                  /* name */
    _SKIP_STR;                  /* owner */
    _SKIP_STR;                  /* group */
    _SKIP_STR;                  /* mname */
    if (extension) {
        _EXTRACT_STR(MAXPATHLEN, extension);
    }
    else
    {
        _SKIP_STR;
    }
    _EXTRACT(uid);
    _EXTRACT(gid);
    /* ignore mid */

    return true;
}

static inline void
ninep_make_Twstat(struct ninep_large_buf *buf,
                  ninep_fid_t fid,
                  ninep_perm_t perm,
                  u_int32_t atime,
                  u_int32_t mtime,
                  u_int64_t size,
                  const char *name)
{
    uint16_t len = 61;

    if (name)
    {
        len += strlen(name);
    }

    _PREPARE_PACK(buf);

    _PACK_TYPE(Twstat);
    _PACK_TAG(0x0001);
    _PACK(fid);
    _PACK(len);             /* struct size */
    _PACK(~(u_int16_t)0);   /* type */
    _PACK(~(u_int32_t)0);   /* dev */
    _PACK(~(u_int8_t)0);    /* qid.type */
    _PACK(~(u_int32_t)0);   /* qid.version */
    _PACK(~(u_int64_t)0);   /* qid.path */
    _PACK(perm);
    _PACK(atime);
    _PACK(mtime);
    _PACK(size);
    if (name)
    {
        _PACK_STR(name);
    }
    else
    {
        _PACK_STR("");
    }
    _PACK_STR("");          /* owner */
    _PACK_STR("");          /* group */
    _PACK_STR("");          /* mname */
    _PACK_STR("");          /* extension */
    _PACK(~(u_int32_t)0);   /* uid */
    _PACK(~(u_int32_t)0);   /* gid */
    _PACK(~(u_int32_t)0);   /* mid */

    _FINALIZE(buf);
}

/* generic T packer for requests that just take a fid */
static inline void
ninep_make_T(ninep_T_t type,
             struct ninep_small_buf *buf,
             ninep_fid_t fid)
{
    _PREPARE_PACK(buf);

    _PACK_TYPE(type);
    _PACK_TAG(0x0001);
    _PACK(fid);

    _FINALIZE(buf);
}

/* generic R checker for replies that carry no data */
static inline bool
ninep_check_R(ninep_T_t type, const struct ninep_small_buf *buf)
{
    _PREPARE_CHECK(buf);

    _CHECK_REPLY_TYPE(Twalk);
    _CHECK_TAG(0x0001);

    return true;
}

#endif /* 9p_proto.h */
