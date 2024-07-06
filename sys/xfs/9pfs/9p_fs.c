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
 * Derived from the XFS skeleton by Frank Naumann, <fnaumann@freemint.de>
 */

/*
 * TODO:
 *  - message packer
 *  - protocol negotiation / attach
 *  - reentrancy
 *
 * Remember:
 *  - everything on the wire is little-endian
 *  - never test for an error condition you can't handle
 */

# include "mint/mint.h"

# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/endian.h"  // <- for le2cpu/cpu2le, be2cpu/cpu2be
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/proc.h"
# include "mint/stat.h"

# include "libkern/libkern.h"

#include "9p.h"
#include "virtio.h"

/*
 * debugging stuff
 */

# if 1
# define FS_DEBUG 1
# endif

/*
 * version
 */

# define VER_MAJOR  0
# define VER_MINOR  0

# if 0
# define ALPHA
# endif

# if 0
# define BETA
# endif

/*
 * startup messages
 */

# define MSG_VERSION    str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE  __DATE__

# define MSG_BOOT   \
    "\033p 9P filesystem driver version " MSG_VERSION " \033q\r\n"


/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *KERNEL;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * filesystem
 */

static long _cdecl ninep_root       (int drv, fcookie *fc);

static long _cdecl ninep_lookup     (fcookie *dir, const char *name, fcookie *fc);
static DEVDRV * _cdecl ninep_getdev (fcookie *fc, long *devsp);
static long _cdecl ninep_getxattr   (fcookie *fc, XATTR *xattr);
static long _cdecl ninep_stat64     (fcookie *fc, STAT *stat);

static long _cdecl ninep_chattr     (fcookie *fc, int attrib);
static long _cdecl ninep_chown      (fcookie *fc, int uid, int gid);
static long _cdecl ninep_chmode     (fcookie *fc, unsigned mode);

static long _cdecl ninep_mkdir      (fcookie *dir, const char *name, unsigned mode);
static long _cdecl ninep_rmdir      (fcookie *dir, const char *name);
static long _cdecl ninep_creat      (fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long _cdecl ninep_remove     (fcookie *dir, const char *name);
static long _cdecl ninep_getname    (fcookie *root, fcookie *dir, char *pathname, int size);
static long _cdecl ninep_rename     (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long _cdecl ninep_opendir    (DIR *dirh, int flags);
static long _cdecl ninep_readdir    (DIR *dirh, char *nm, int nmlen, fcookie *);
static long _cdecl ninep_rewinddir  (DIR *dirh);
static long _cdecl ninep_closedir   (DIR *dirh);

static long _cdecl ninep_pathconf   (fcookie *dir, int which);
static long _cdecl ninep_dfree      (fcookie *dir, long *buf);
static long _cdecl ninep_writelabel (fcookie *dir, const char *name);
static long _cdecl ninep_readlabel  (fcookie *dir, char *name, int namelen);

static long _cdecl ninep_symlink    (fcookie *dir, const char *name, const char *to);
static long _cdecl ninep_readlink   (fcookie *file, char *buf, int len);
static long _cdecl ninep_hardlink   (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long _cdecl ninep_fscntl     (fcookie *dir, const char *name, int cmd, long arg);
static long _cdecl ninep_dskchng    (int drv, int mode);

static long _cdecl ninep_release    (fcookie *fc);
static long _cdecl ninep_dupcookie  (fcookie *dst, fcookie *src);
static long _cdecl ninep_sync       (void);

static FILESYS ftab =
{
    NULL,

    /*
     * FS_KNOPARSE      kernel shouldn't do parsing
     * FS_CASESENSITIVE file names are case sensitive
     * FS_NOXBIT        if a file can be read, it can be executed
     * FS_LONGPATH      file system understands "size" argument to "getname"
     * FS_NO_C_CACHE    don't cache cookies for this filesystem
     * FS_DO_SYNC       file system has a sync function
     * FS_OWN_MEDIACHANGE   filesystem control self media change (dskchng)
     * FS_REENTRANT_L1  fs is level 1 reentrant
     * FS_REENTRANT_L2  fs is level 2 reentrant
     * FS_EXT_1     extensions level 1 - mknod & unmount
     * FS_EXT_2     extensions level 2 - additional place at the end
     * FS_EXT_3     extensions level 3 - stat & native UTC timestamps
     */
    FS_CASESENSITIVE    |
    FS_LONGPATH         |
    FS_NO_C_CACHE       |
    FS_OWN_MEDIACHANGE  |
//  FS_REENTRANT_L1     |
//  FS_REENTRANT_L2     |
//  FS_EXT_1        |
//  FS_EXT_2        |
    FS_EXT_3        ,

    ninep_root,
    ninep_lookup, ninep_creat, ninep_getdev, ninep_getxattr,
    ninep_chattr, ninep_chown, ninep_chmode,
    ninep_mkdir, ninep_rmdir, ninep_remove, ninep_getname, ninep_rename,
    ninep_opendir, ninep_readdir, ninep_rewinddir, ninep_closedir,
    ninep_pathconf, ninep_dfree, ninep_writelabel, ninep_readlabel,
    ninep_symlink, ninep_readlink, ninep_hardlink, ninep_fscntl, ninep_dskchng,
    ninep_release, ninep_dupcookie,
    ninep_sync,

    /* FS_EXT_1 */
    NULL, NULL,

    /* FS_EXT_2
     */

    /* FS_EXT_3 */
    ninep_stat64,

    0, 0, 0, 0, 0,
    NULL, NULL
};

/*
 * device driver
 */

static long _cdecl ninep_open       (FILEPTR *f);
static long _cdecl ninep_write      (FILEPTR *f, const char *buf, long bytes);
static long _cdecl ninep_read       (FILEPTR *f, char *buf, long bytes);
static long _cdecl ninep_lseek      (FILEPTR *f, long where, int whence);
static long _cdecl ninep_ioctl      (FILEPTR *f, int mode, void *buf);
static long _cdecl ninep_datime     (FILEPTR *f, ushort *time, int flag);
static long _cdecl ninep_close      (FILEPTR *f, int pid);
static long _cdecl null_select      (FILEPTR *f, long int p, int mode);
static void _cdecl null_unselect    (FILEPTR *f, long int p, int mode);

static DEVDRV devtab =
{
    ninep_open,
    ninep_write, ninep_read, ninep_lseek, ninep_ioctl, ninep_datime,
    ninep_close,
    null_select, null_unselect
};


#define _drvbits        *(unsigned long *)0x4C2
#define DRIVE_NUM       ('V' - 'A')
#define DRIVE_BIT       (1UL << DRIVE_NUM)

/*
 * debugging stuff
 */

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


/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

INLINE ulong
current_time (void)
{
    return utc.tv_sec;
}
# define CURRENT_TIME   current_time ()

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN init & configuration part */
FILESYS * _cdecl init_xfs (struct kerinfo *k)
{
    KERNEL = k;

    c_conws (MSG_BOOT);
    c_conws ("\r\n");

    KERNEL_DEBUG (("9p.c: init"));

    /* version check */
    if ((MINT_MAJOR < 1) || 
        (MINT_MAJOR == 1 && MINT_MINOR < 15) ||
        (MINT_KVERSION < 1) ||
        (bio.version != 3) ||
        (bio.revision < 1))
    {
        ALERT(("Unsupported FreeMiNT version, not loading."));
        return NULL;
    }

    /* check our drive is available */
    if (_drvbits & DRIVE_BIT) {
        ALERT(("Drive V: already in use, not loading."));
        return NULL;
    }

    /* set up the virtio-9p transport */
    if (virtio_init())
    {
        ALERT(("Virtio transport setup failure, not loading."));
        return NULL;
    }

    /* attach the share */
    if (!ninep_attach()) {
        ALERT(("9pfs: attach failed"));
        for (;;);
        return NULL;
    }


    /* Add our drive */
    _drvbits |= DRIVE_BIT;

    KERNEL_DEBUG ("9p: loaded and ready (k = %lx) -> %lx.", (unsigned long)k, (long) &ftab);
    return &ftab;
}

/* END init & configuration part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN filesystem */

static long _cdecl
ninep_root (int drv, fcookie *fc)
{
    /* is this us? */
    if (drv != DRIVE_NUM) {
        return ENXIO;
    }

    return E_OK;
}

static long _cdecl
ninep_lookup (fcookie *dir, const char *name, fcookie *fc)
{
//  COOKIE *c = (COOKIE *) dir->index;
//  SI *s = super [dir->dev];

    DEBUG (("dummy [%c]: ninep_lookup (%s)", dir->dev+'A', name));

    *fc = *dir;
    
    /* 1 - itself */
    if (!*name || (name [0] == '.' && name [1] == '\0'))
    {   
//      c->links++;
    
        DEBUG (("dummy [%c]: ninep_lookup: leave ok, (name = \".\")", dir->dev+'A'));
        return E_OK;
    }
    
    /* 2 - parent dir */
    if (name [0] == '.' && name [1] == '.' && name [2] == '\0')
    {
//      if (dir == rootcookie)
//      {
//          DEBUG (("dummy [%c]: ninep_lookup: leave ok, EMOUNT, (name = \"..\")", dir->dev+'A'));
//          return EMOUNT;
//      }
    }
    
    /* 3 - normal entry */
    {
//      if (not found)
            return ENOENT;
    }
    
    DEBUG (("dummy [%c]: ninep_lookup: leave ok", dir->dev+'A'));
    return E_OK;
}

static DEVDRV * _cdecl
ninep_getdev (fcookie *fc, long *devsp)
{
    if (fc->fs == &ftab)
        return &devtab;
    
    *devsp = ENOSYS;
    return NULL;
}

static long _cdecl
ninep_getxattr (fcookie *fc, XATTR *xattr)
{
    return ENOSYS;
    
# if 0
    xattr->mode         = 
    xattr->index            = 
    xattr->dev          = 
    xattr->rdev             = 
    xattr->nlink            = 
    xattr->uid          = 
    xattr->gid          = 
    xattr->size             = 
    xattr->blksize          = 
    xattr->nblocks          = /* number of blocks of size 'blksize' */
    *((long *) &(xattr->mtime)) = 
    *((long *) &(xattr->atime)) = 
    *((long *) &(xattr->ctime)) = 
# endif
    
    /* fake attr field a little bit */
    if (S_ISDIR (xattr->mode))
    {
        xattr->attr = FA_DIR;
    }
    else
        xattr->attr = (xattr->mode & 0222) ? 0 : FA_RDONLY;
    
    return E_OK;
}

static long _cdecl
ninep_stat64 (fcookie *fc, STAT *stat)
{
    /* later */
    return ENOSYS;
}

static long _cdecl
ninep_chattr (fcookie *fc, int attrib)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_chown (fcookie *fc, int uid, int gid)
{
    /* nothing todo */
    return ENOSYS;
}

static long _cdecl
ninep_chmode (fcookie *fc, unsigned mode)
{
    /* nothing todo */
    return ENOSYS;
}

static long _cdecl
ninep_mkdir (fcookie *dir, const char *name, unsigned mode)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_rmdir (fcookie *dir, const char *name)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_creat (fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_remove (fcookie *dir, const char *name)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
    DEBUG (("dummy: ninep_getname enter"));
    
    pathname [0] = '\0';
    
    {
        ;
    }
    
    pathname [0] = '\0';
    
    DEBUG (("dummy: ninep_getname: path not found?"));
    return ENOTDIR;
}

static long _cdecl
ninep_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_opendir (DIR *dirh, int flags)
{
//  if (!S_ISDIR (...))
    {
        DEBUG (("dummy: ninep_opendir: dir not a DIR!"));
        return EACCES;
    }
    
    dirh->index = 0;
    return E_OK;
}

static long _cdecl
ninep_readdir (DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
    long ret = ENMFILES;
    
    {
        ;
    }
    
    return ret;
}

static long _cdecl
ninep_rewinddir (DIR *dirh)
{
    {
        ;
    }
    
    dirh->index = 0;
    return E_OK;
}

static long _cdecl
ninep_closedir (DIR *dirh)
{
    {
        ;
    }
        
    dirh->index = 0;
    return E_OK;
}

static long _cdecl
ninep_pathconf (fcookie *dir, int which)
{
    switch (which)
    {
        case DP_INQUIRE:    return DP_VOLNAMEMAX;
        case DP_IOPEN:      return UNLIMITED;
        case DP_MAXLINKS:   return UNLIMITED;
        case DP_PATHMAX:    return UNLIMITED;
        case DP_NAMEMAX:    return 255;     /* correct me */
        case DP_ATOMIC:     return 1024;        /* correct me */
        case DP_TRUNC:      return DP_NOTRUNC;
        case DP_CASE:       return DP_CASEINSENS;   /* correct me */
        case DP_MODEATTR:   return (DP_ATTRBITS /* correct me */
                        | DP_MODEBITS
                        | DP_FT_DIR
                        | DP_FT_REG
                        | DP_FT_LNK
                    );
        case DP_XATTRFIELDS:    return (DP_INDEX    /* correct me */
                        | DP_DEV
                        | DP_NLINK
                        | DP_UID
                        | DP_GID
                        | DP_BLKSIZE
                        | DP_SIZE
                        | DP_NBLOCKS
                        | DP_ATIME
                        | DP_CTIME
                        | DP_MTIME
                    );
        case DP_VOLNAMEMAX: return 255;     /* correct me */
    }
    
    return ENOSYS;
}

static long _cdecl
ninep_dfree (fcookie *dir, long *buf)
{
    DEBUG (("dummy: ninep_dfree called"));
    
    *buf++  = 0;    /* free cluster */
    *buf++  = 0;    /* cluster count */
    *buf++  = 2048; /* sectorsize */
    *buf    = 1;    /* nr of sectors per cluster */
    
    return E_OK;
}

static long _cdecl
ninep_writelabel (fcookie *dir, const char *name)
{
    /* nothing todo */
    return EACCES;
}

static long _cdecl
ninep_readlabel (fcookie *dir, char *name, int namelen)
{
    /* cosmetical */
    
    {
        ;
    }
    
    return EBADARG;
}

static long _cdecl
ninep_symlink (fcookie *dir, const char *name, const char *to)
{
    /* nothing todo */
    return ENOSYS;
}

static long _cdecl
ninep_readlink (fcookie *file, char *buf, int len)
{
    long ret = ENOSYS;
    
//  if (S_ISLNK (...))
    {
        ;
    }
    
    return ret;
}

static long _cdecl
ninep_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
    /* nothing todo */
    return ENOSYS;
}

static long _cdecl
ninep_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
    switch (cmd)
    {
        case MX_KER_XFSNAME:
        {
            strcpy ((char *) arg, "dummy");
            return E_OK;
        }
        case FS_INFO:
        {
            struct fs_info *info = (struct fs_info *) arg;
            if (info)
            {
                strcpy (info->name, "dummy-xfs");
                info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
                // XXX
                info->type = FS_ISO9660;
                strcpy (info->type_asc, "dummy");
                
                /* more types later */
            }
            return E_OK;
        }
        case FS_USAGE:
        {
            struct fs_usage *usage = (struct fs_usage *) arg;
            if (usage)
            {
# if 0
                usage->blocksize = ;
                usage->blocks = ;
                usage->free_blocks = ;
                usage->inodes = FS_UNLIMITED;
                usage->free_inodes = FS_UNLIMITED;
# endif
            }
            return E_OK;
        }
    }
    
    return ENOSYS;
}

static long _cdecl
ninep_dskchng (int drv, int mode)
{
    return 0;
}

static long _cdecl
ninep_release (fcookie *fc)
{
    /* this function decrease the inode reference counter
     * if reached 0 this inode is no longer used by the kernel
     */
//  COOKIE *c = (COOKIE *) fc->index;
    
//  c->links--;
    return E_OK;
}

static long _cdecl
ninep_dupcookie (fcookie *dst, fcookie *src)
{
    /* this function increase the inode reference counter
     * kernel use this to create a new reference to an inode
     * and to verify that the inode remain valid until it is
     * released
     */
//  COOKIE *c = (COOKIE *) src->index;
    
//  c->links++;
    *dst = *src;
    
    return E_OK;
}

static long _cdecl
ninep_sync (void)
{
    return E_OK;
}

/* END filesystem */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver */

static long _cdecl
ninep_open (FILEPTR *f)
{
//  COOKIE *c = (COOKIE *) f->fc.index;
    
    DEBUG (("dummy: ninep_open: enter"));
    
//  if (!S_ISREG (...))
    {
        DEBUG (("dummy: ninep_open: leave failure, not a valid file"));
        return EACCES;
    }
    
    if (((f->flags & O_RWMODE) == O_WRONLY)
        || ((f->flags & O_RWMODE) == O_RDWR))
    {
        return EROFS;
    }
    
//  if (c->open && denyshare (c->open, f))
//  {
//      DEBUG (("dummy: ninep_open: file sharing denied"));
//      return EACCES;
//  }
    
    f->pos = 0;
    f->devinfo = 0;
//  f->next = c->open;
//  c->open = f;
    
//  c->links++;
    
    DEBUG (("dummy: ninep_open: leave ok"));
    return E_OK;
}

static long _cdecl
ninep_write (FILEPTR *f, const char *buf, long bytes)
{
    /* nothing todo */
    return 0;
}

static long _cdecl
ninep_read (FILEPTR *f, char *buf, long bytes)
{
    return 0;
}

static long _cdecl
ninep_lseek (FILEPTR *f, long where, int whence)
{
//  COOKIE *c = (COOKIE *) f->fc.index;
    
    DEBUG (("dummy: ninep_lseek: enter (where = %li, whence = %i)", where, whence));
    
    switch (whence)
    {
        case SEEK_SET:              break;
        case SEEK_CUR:  where += f->pos;    break;
//      case SEEK_END:  where += c->stat.size;  break;
        default:    return EINVAL;
    }
    
    if (where < 0)
    {
        DEBUG (("dummy: ninep_lseek: leave failure EBADARG (where = %li)", where));
        return EBADARG;
    }
    
    f->pos = where;
    
    DEBUG (("dummy: ninep_lseek: leave ok (f->pos = %li)", f->pos));
    return where;
}

static long _cdecl
ninep_ioctl (FILEPTR *f, int mode, void *buf)
{
//  COOKIE *c = (COOKIE *) f->fc.index;
    
    DEBUG (("fnramfs: ninep_ioctl: enter (mode = %i)", mode));
    
    switch (mode)
    {
        case FIONREAD:
        {
            *(long *) buf = 0; //c->stat.size - f->pos;
            return E_OK;
        }
        case FIONWRITE:
        {
            *(long *) buf = 0;
            return E_OK;
        }
        case FIOEXCEPT:
        {
            *(long *) buf = 0;
            return E_OK;
        }
# if 0
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *fl = (struct flock *) buf;
            
            LOCK t;
            LOCK *lck;
            
            int cpid;
                
            t.l = *fl;
            
            switch (t.l.l_whence)
            {
                case SEEK_SET:
                {
                    break;
                }
                case SEEK_CUR:
                {
                    long r = ninep_lseek (f, 0L, SEEK_CUR);
                    t.l.l_start += r;
                    break;
                }
                case SEEK_END:
                {
                    long r = ninep_lseek (f, 0L, SEEK_CUR);
                    t.l.l_start = ninep_lseek (f, t.l.l_start, SEEK_END);
                    (void) ninep_lseek (f, r, SEEK_SET);
                    break;
                }
                default:
                {
                    DEBUG (("ninep_ioctl: invalid value for l_whence\n"));
                    return ENOSYS;
                }
            }
            
            if (t.l.l_start < 0) t.l.l_start = 0;
            t.l.l_whence = 0;
            
            cpid = p_getpid ();
            
            if (mode == F_GETLK)
            {
                lck = denylock (cpid, c->locks, &t);
                if (lck)
                    *fl = lck->l;
                else
                    fl->l_type = F_UNLCK;
                
                return E_OK;
            }
            
            if (t.l.l_type == F_UNLCK)
            {
                /* try to find the lock */
                LOCK **lckptr = &(c->locks);
                
                lck = *lckptr;
                while (lck)
                {
                    if (lck->l.l_pid == cpid
                                        && ((lck->l.l_start == t.l.l_start
                             && lck->l.l_len == t.l.l_len) ||
                            (lck->l.l_start >= t.l.l_start
                             && t.l.l_len == 0)))
                    {
                        /* found it -- remove the lock */
                        *lckptr = lck->next;
                        
                        DEBUG (("ninep_ioctl: unlocked %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
                        
                        /* wake up anyone waiting on the lock */
                        wake (IO_Q, (long) lck);
                        kfree (lck, sizeof (*lck));
                        
                        return E_OK;
                    }
                    
                    lckptr = &(lck->next);
                    lck = lck->next;
                }
                
                return ENSLOCK;
            }
            
            DEBUG (("ninep_ioctl: lock %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
            
            /* see if there's a conflicting lock */
            while ((lck = denylock (cpid, c->locks, &t)) != 0)
            {
                DEBUG (("ninep_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
                if (mode == F_SETLKW)
                {
                    /* sleep a while */
                    sleep (IO_Q, (long) lck);
                }
                else
                    return ELOCKED;
            }
            
            /* if not, add this lock to the list */
            lck = kmalloc (sizeof (*lck));
            if (!lck)
            {
                ALERT (("dummy.c: kmalloc fail in: ninep_ioctl (%lx)", c));
                return ENOMEM;
            }
            
            lck->l = t.l;
            lck->l.l_pid = cpid;
            
            lck->next = c->locks;
            c->locks = lck;
            
            /* mark the file as being locked */
            f->flags |= O_LOCK;
            return E_OK;
        }
# endif
    }
    
    return ENOSYS;
}

static long _cdecl
ninep_datime (FILEPTR *f, ushort *time, int flag)
{
//  COOKIE *c = (COOKIE *) f->fc.index;
    
    switch (flag)
    {
        case 0:
//          *(ulong *) time = c->stat.mtime.time;
            break;
        
        case 1:
            return EROFS;
        
        default:
            return EBADARG;
    }
    
    return E_OK;
}

static long _cdecl
ninep_close (FILEPTR *f, int pid)
{
//  COOKIE *c = (COOKIE *) f->fc.index;
    
    DEBUG (("dummy: ninep_close: enter (f->links = %i)", f->links));
    
# if 0
    /* if a lock was made, remove any locks of the process */
    if (f->flags & O_LOCK)
    {
        LOCK *lock;
        LOCK **oldlock;
        
        DEBUG (("fnramfs: ninep_close: remove lock (pid = %i)", pid));
        
        oldlock = &c->locks;
        lock = *oldlock;
        
        while (lock)
        {
            if (lock->l.l_pid == pid)
            {
                *oldlock = lock->next;
                /* (void) ninep_lock ((int) f->devinfo, 1, lock->l.l_start, lock->l.l_len); */
                wake (IO_Q, (long) lock);
                kfree (lock, sizeof (*lock));
            }
            else
            {
                oldlock = &lock->next;
            }
            
            lock = *oldlock;
        }
    }
    
    if (f->links <= 0)
    {
        /* remove the FILEPTR from the linked list */
        register FILEPTR **temp = &c->open;
        register long flag = 1;
        
        while (*temp)
        {
            if (*temp == f)
            {
                *temp = f->next;
                f->next = NULL;
                flag = 0;
                break;
            }
            temp = &(*temp)->next;
        }
        
        if (flag)
        {
            ALERT (("dummy: ninep_close: remove open FILEPTR failed"));
        }
        
        c->links--;
    }
# endif
    
    DEBUG (("dummy: ninep_close: leave ok"));
    return E_OK;
}

static long _cdecl
null_select (FILEPTR *f, long int p, int mode)
{
    if ((mode == O_RDONLY) || (mode == O_WRONLY))
        /* we're always ready to read/write */
        return 1;
    
    /* other things we don't care about */
    return E_OK;
}

static void _cdecl
null_unselect (FILEPTR *f, long int p, int mode)
{
}

/* END device driver */
/****************************************************************************/
