/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _9p_proto_h
# define _9p_proto_h

/* T (transaction) types */
enum
{
    Tversion    = 100,
    Tauth       = 102,
    Tattach     = 104,
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
};

/* unique R (reply) types */
enum
{
    Rerror      = 107,
};

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



struct ninep_t_attach
{
    struct ninep_hdr    hdr;
    u_int32_t           fid;
    u_int32_t           afid;
    u_int16_t           uname_len;
    u_int16_t           aname_len;
};

struct ninep_r_attach
{
    struct ninep_hdr    hdr;
    struct ninep_qid    qid;
};

#endif /* 9p_proto.h */
