/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef TX4938_TX4938_MIPS_H
#define TX4938_TX4938_MIPS_H
#ifndef __ASSEMBLY__

#define reg_rd08(r) ((u8 )(*((vu8 *)(r))))
#define reg_rd16(r) ((u16)(*((vu16*)(r))))
#define reg_rd32(r) ((u32)(*((vu32*)(r))))
#define reg_rd64(r) ((u64)(*((vu64*)(r))))

#define reg_wr08(r, v) ((*((vu8 *)(r)))=((u8 )(v)))
#define reg_wr16(r, v) ((*((vu16*)(r)))=((u16)(v)))
#define reg_wr32(r, v) ((*((vu32*)(r)))=((u32)(v)))
#define reg_wr64(r, v) ((*((vu64*)(r)))=((u64)(v)))

typedef volatile __signed char vs8;
typedef volatile unsigned char vu8;

typedef volatile __signed short vs16;
typedef volatile unsigned short vu16;

typedef volatile __signed int vs32;
typedef volatile unsigned int vu32;

typedef s8 s08;
typedef vs8 vs08;

typedef u8 u08;
typedef vu8 vu08;

#if _MIPS_SZLONG == 64

typedef volatile __signed__ long vs64;
typedef volatile unsigned long vu64;

#else

typedef volatile __signed__ long long vs64;
typedef volatile unsigned long long vu64;

#endif
#endif
#endif
