#ifndef MINTBOOT_MB_OSBIND_H
#define MINTBOOT_MB_OSBIND_H

#ifdef __cplusplus
extern "C" {
#endif

#define MB_TRAP_CLOBBERS "d1", "d2", "a0", "a1", "a2", "cc", "memory"

#define MB_TRAP1_W(n)                                                      \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		__asm__ volatile(                                            \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addql	#2,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n)                                             \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WW(n, a)                                                   \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		__asm__ volatile(                                            \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addql	#4,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WL(n, a)                                                   \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		__asm__ volatile(                                            \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WLW(n, a, b)                                               \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		short _b = (short)(b);                                      \
		__asm__ volatile(                                            \
			"movw	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addql	#8,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WWLL(n, a, b, c)                                          \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		long _b = (long)(b);                                        \
		long _c = (long)(c);                                        \
		__asm__ volatile(                                            \
			"movl	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#12,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WLWW(n, a, b, c)                                          \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		short _b = (short)(b);                                      \
		short _c = (short)(c);                                      \
		__asm__ volatile(                                            \
			"movw	%4,%%sp@-\n\t"                                \
			"movw	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#10,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WWW(n, a, b)                                              \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		short _b = (short)(b);                                      \
		__asm__ volatile(                                            \
			"movw	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WWWLL(n, a, b, c, d)                                      \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		short _b = (short)(b);                                      \
		long _c = (long)(c);                                        \
		long _d = (long)(d);                                        \
		__asm__ volatile(                                            \
			"movl	%5,%%sp@-\n\t"                                \
			"movl	%4,%%sp@-\n\t"                                \
			"movw	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c), "r"(_d)         \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WWLW(n, a, b, c)                                          \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		long _b = (long)(b);                                        \
		short _c = (short)(c);                                      \
		__asm__ volatile(                                            \
			"movw	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#10,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WLL(n, a, b)                                               \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		__asm__ volatile(                                            \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#10,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP1_WLLL(n, a, b, c)                                           \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		long _c = (long)(c);                                        \
		__asm__ volatile(                                            \
			"movl	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#1\n\t"                                     \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_W(n)                                                     \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		__asm__ volatile(                                            \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addql	#2,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n)                                             \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WW(n, a)                                                 \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		__asm__ volatile(                                            \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addql	#4,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WWW(n, a, b)                                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		short _b = (short)(b);                                      \
		__asm__ volatile(                                            \
			"movw	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WL(n, a)                                                 \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		__asm__ volatile(                                            \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WLW(n, a, b)                                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		long _b = (long)(b);                                        \
		__asm__ volatile(                                            \
			"movl	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addql	#8,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WLL(n, a, b)                                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		__asm__ volatile(                                            \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addl	#10,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP13_WWLWWW(n, a, b, c, d, e)                                 \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		long _b = (long)(b);                                        \
		short _c = (short)(c);                                      \
		short _d = (short)(d);                                      \
		short _e = (short)(e);                                      \
		__asm__ volatile(                                            \
			"movw	%6,%%sp@-\n\t"                                \
			"movw	%5,%%sp@-\n\t"                                \
			"movw	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#13\n\t"                                    \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c), "r"(_d), "r"(_e) \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define Pterm0()                                                           \
	(long)MB_TRAP1_W((short)(0x00))

#define Cconin()                                                           \
	(long)MB_TRAP1_W((short)(0x01))

#define Cconws(buf)                                                        \
	(long)MB_TRAP1_WL((short)(0x09), (long)(buf))

#define Super(stack)                                                       \
	(long)MB_TRAP1_WL((short)(0x20), (long)(stack))

#define Dgetdrv()                                                          \
	(long)MB_TRAP1_W((short)(0x19))

#define Fsetdta(dta)                                                       \
	(long)MB_TRAP1_WL((short)(0x1a), (long)(dta))

#define Dsetdrv(drive)                                                     \
	(long)MB_TRAP1_WW((short)(0x0e), (short)(drive))

#define Dgetpath(buf, d)                                                   \
	(long)MB_TRAP1_WLW((short)(0x47), (long)(buf), (short)(d))

#define Dfree(buf, d)                                                      \
	(long)MB_TRAP1_WLW((short)(0x36), (long)(buf), (short)(d))

#define Dsetpath(path)                                                     \
	(long)MB_TRAP1_WL((short)(0x3b), (long)(path))

#define Fcreate(fn, mode)                                                  \
	(long)MB_TRAP1_WLW((short)(0x3c), (long)(fn), (short)(mode))

#define Rwabs(rwflag, buf, n, sector, d)                                   \
	(long)MB_TRAP13_WWLWWW((short)(0x04), (short)(rwflag),            \
			       (long)(buf), (short)(n), (short)(sector),  \
			       (short)(d))

#define Setexc(vnum, vptr)                                                 \
	(void (*)(void))MB_TRAP13_WLW((short)(0x05), (short)(vnum),       \
				      (long)(vptr))

#define Getbpb(d)                                                          \
	(void *)MB_TRAP13_WW((short)(0x07), (short)(d))

#define Bconstat(dev)                                                      \
	(short)MB_TRAP13_WW((short)(0x01), (short)(dev))

#define Bconin(dev)                                                        \
	(long)MB_TRAP13_WW((short)(0x02), (short)(dev))

#define Bcostat(dev)                                                       \
	(short)MB_TRAP13_WW((short)(0x08), (short)(dev))

#define Drvmap()                                                           \
	(long)MB_TRAP13_W((short)(0x0A))

#define Kbshift(data)                                                      \
	(long)MB_TRAP13_WW((short)(0x0B), (short)(data))

#define Bconout(dev, c)                                                    \
	(long)MB_TRAP13_WWW((short)(0x03), (short)(dev), (short)((c) & 0xff))

#define Fopen(fn, mode)                                                    \
	(long)MB_TRAP1_WLW((short)(0x3D), (long)(fn), (short)(mode))

#define Fclose(handle)                                                     \
	(long)MB_TRAP1_WW((short)(0x3e), (short)(handle))

#define Fread(handle, cnt, buf)                                            \
	(long)MB_TRAP1_WWLL((short)(0x3f), (short)(handle), (long)(cnt),  \
			    (long)(buf))

#define Fwrite(handle, cnt, buf)                                           \
	(long)MB_TRAP1_WWLL((short)(0x40), (short)(handle), (long)(cnt),  \
			    (long)(buf))

#define Fdelete(fn)                                                        \
	(long)MB_TRAP1_WL((short)(0x41), (long)(fn))

#define Fseek(where, handle, how)                                          \
	(long)MB_TRAP1_WLWW((short)(0x42), (long)(where),                \
			    (short)(handle), (short)(how))

#define Mxalloc(amt, flag)                                                 \
	(long)MB_TRAP1_WLW((short)(0x44), (long)(amt), (short)(flag))

#define Fgetdta()                                                          \
	(void *)MB_TRAP1_W((short)(0x2f))

#define Mshrink(ptr, size)                                                 \
	(long)MB_TRAP1_WWLL((short)(0x4a), (short)0, (long)(ptr),        \
			    (long)(size))

#define Flock(handle, mode, start, length)                                 \
	(long)MB_TRAP1_WWWLL((short)(0x5c), (short)(handle),             \
			     (short)(mode), (long)(start),           \
			     (long)(length))

#define Fcntl(handle, arg, cmd)                                            \
	(long)MB_TRAP1_WWLW((short)(0x52), (short)(handle),              \
			    (long)(arg), (short)(cmd))

#define MB_TRAP14_W(n)                                                     \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		__asm__ volatile(                                            \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addql	#2,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n)                                             \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WW(n, a)                                                 \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		__asm__ volatile(                                            \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addql	#4,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WL(n, a)                                                 \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		__asm__ volatile(                                            \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a)                                   \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WLL(n, a, b)                                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		__asm__ volatile(                                            \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addl	#10,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WWLL(n, a, b, c)                                         \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		long _b = (long)(b);                                        \
		long _c = (long)(c);                                        \
		__asm__ volatile(                                            \
			"movl	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addl	#12,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WWW(n, a, b)                                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		short _b = (short)(b);                                      \
		__asm__ volatile(                                            \
			"movw	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addql	#6,%%sp\n\t"                                \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b)                           \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WWWWWWW(n, a, b, c, d, e, f)                             \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		short _a = (short)(a);                                      \
		short _b = (short)(b);                                      \
		short _c = (short)(c);                                      \
		short _d = (short)(d);                                      \
		short _e = (short)(e);                                      \
		short _f = (short)(f);                                      \
		__asm__ volatile(                                            \
			"movw	%7,%%sp@-\n\t"                                \
			"movw	%6,%%sp@-\n\t"                                \
			"movw	%5,%%sp@-\n\t"                                \
			"movw	%4,%%sp@-\n\t"                                \
			"movw	%3,%%sp@-\n\t"                                \
			"movw	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c), "r"(_d),         \
			  "r"(_e), "r"(_f)                                \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WLLL(n, a, b, c)                                         \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		long _c = (long)(c);                                        \
		__asm__ volatile(                                            \
			"movl	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c)                  \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define MB_TRAP14_WLLWW(n, a, b, c, d)                                     \
	__extension__({                                                     \
		register long __retvalue __asm__("d0");                     \
		long _a = (long)(a);                                        \
		long _b = (long)(b);                                        \
		short _c = (short)(c);                                      \
		short _d = (short)(d);                                      \
		__asm__ volatile(                                            \
			"movw	%5,%%sp@-\n\t"                                \
			"movw	%4,%%sp@-\n\t"                                \
			"movl	%3,%%sp@-\n\t"                                \
			"movl	%2,%%sp@-\n\t"                                \
			"movw	%1,%%sp@-\n\t"                                \
			"trap	#14\n\t"                                    \
			"addl	#14,%%sp\n\t"                               \
			: "=r"(__retvalue)                                  \
			: "g"(n), "r"(_a), "r"(_b), "r"(_c), "r"(_d)         \
			: MB_TRAP_CLOBBERS);                                \
		__retvalue;                                                 \
	})

#define Initmous(type, param, vptr)                                        \
	(void)MB_TRAP14_WWLL((short)(0x00), (short)(type),               \
			     (long)(param), (long)(vptr))

#define Physbase()                                                         \
	(void *)MB_TRAP14_W((short)(0x02))

#define Getrez()                                                           \
	(short)MB_TRAP14_W((short)(0x04))

#define Vsetscreen(lscrn, pscrn, rez, mode)                                \
	(long)MB_TRAP14_WLLWW((short)(0x05), (long)(lscrn),              \
			      (long)(pscrn), (short)(rez),            \
			      (short)(mode))

#define Iorec(dev)                                                         \
	(void *)MB_TRAP14_WW((short)(0x0e), (short)(dev))

#define Rsconf(baud, flow, uc, rs, ts, sc)                                 \
	(long)MB_TRAP14_WWWWWWW((short)(0x0f), (short)(baud),            \
				(short)(flow), (short)(uc),             \
				(short)(rs), (short)(ts),               \
				(short)(sc))

#define Keytbl(nrml, shft, caps)                                           \
	(void *)MB_TRAP14_WLLL((short)(0x10), (long)(nrml),               \
			       (long)(shft), (long)(caps))

#define Cursconf(rate, attr)                                               \
	(short)MB_TRAP14_WWW((short)(0x15), (short)(rate),               \
			     (short)(attr))

#define Settime(time)                                                      \
	(void)MB_TRAP14_WL((short)(0x16), (long)(time))

#define Gettime()                                                          \
	(long)MB_TRAP14_W((short)(0x17))

#define Bioskeys()                                                         \
	(void)MB_TRAP14_W((short)(0x18))

#define Offgibit(ormask)                                                   \
	(void)MB_TRAP14_WW((short)(0x1d), (short)(ormask))

#define Ongibit(andmask)                                                   \
	(void)MB_TRAP14_WW((short)(0x1e), (short)(andmask))

#define Dosound(ptr)                                                       \
	(void)MB_TRAP14_WL((short)(0x20), (long)(ptr))

#define Kbdvbase()                                                         \
	(void *)MB_TRAP14_W((short)(0x22))

#define Kbrate(delay, reprate)                                             \
	(short)MB_TRAP14_WWW((short)(0x23), (short)(delay),              \
			     (short)(reprate))

#define Vsync()                                                            \
	(void)MB_TRAP14_W((short)(0x25))

#define Supexec(func)                                                      \
	(long)MB_TRAP14_WL((short)(0x26), (long)(func))

#define Bconmap(dev)                                                       \
	(long)MB_TRAP14_WW((short)(0x2C), (short)(dev))

#ifdef __cplusplus
}
#endif

#endif
