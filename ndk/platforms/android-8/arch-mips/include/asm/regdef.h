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
#ifndef _ASM_REGDEF_H
#define _ASM_REGDEF_H

#include <asm/sgidefs.h>

#if _MIPS_SIM == _MIPS_SIM_ABI32

#define zero $0  
#define AT $1  
#define v0 $2  
#define v1 $3
#define a0 $4  
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8  
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16  
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24  
#define t9 $25
#define jp $25  
#define k0 $26  
#define k1 $27
#define gp $28  
#define sp $29  
#define fp $30  
#define s8 $30  
#define ra $31  

#endif

#if (_MIPS_SIM == _MIPS_SIM_ABI64) || (_MIPS_SIM == _MIPS_SIM_NABI32)

#define zero $0  
#define AT $at  
#define v0 $2  
#define v1 $3
#define a0 $4  
#define a1 $5
#define a2 $6
#define a3 $7
#define a4 $8  
#define ta0 $8
#define a5 $9
#define ta1 $9
#define a6 $10
#define ta2 $10
#define a7 $11
#define ta3 $11
#define t0 $12  
#define t1 $13
#define t2 $14
#define t3 $15
#define s0 $16  
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24  
#define t9 $25  
#define jp $25  
#define k0 $26  
#define k1 $27
#define gp $28  
#define sp $29  
#define fp $30  
#define s8 $30  
#define ra $31  

#endif

#endif
