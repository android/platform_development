/*	$OpenBSD: setjmp.h,v 1.2 2004/08/10 21:10:56 pefo Exp $	*/

/* Public domain */

#ifndef _MIPS_SETJMP_H_
#define _MIPS_SETJMP_H_

#ifdef __LP64__
#define	_JBLEN	22		/* size, in 8-byte longs, of a mips64 jmp_buf */
#else
#define	_JBLEN	29		/* size, in 4-byte longs, of a mips32 jmp_buf */
#endif

#endif /* !_MIPS_SETJMP_H_ */
