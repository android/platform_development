/*
 * Copyright (c) 2010 MIPS Technologies, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with
 *        the distribution.
 *      * Neither the name of MIPS Technologies Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MIPS_STRING_OPS_H
#define __MIPS_STRING_OPS_H
    /* This definition of the byte bitfields uses the
       assumption that the layout of the bitfields is
       equivalent to the layout in memory.  Generally,
       for the MIPS ABIs, this is true. If you compile
       the strcmp.c file with -DSMOKE_TEST_NEW_STRCMP,
       this assumption will be tested.

       Also, regardless of char signedness, ANSI C dictates that
       strcmp() treats each character as unsigned char.  For
       strlen and the like, signedness doesn't matter.

       Also, this code assumes that there are 8-bits per 'char'.  */

#if __mips64
typedef struct bits
{
  unsigned B0:8, B1:8, B2:8, B3:8, B4:8, B5:8, B6:8, B7:8;
} bits_t;
#else
typedef struct bits
{
  unsigned B0:8, B1:8, B2:8, B3:8;
} bits_t;
#endif

#ifndef _ULW
    /* for MIPS GCC, there is no unaligned builtins - so this code forces
       the compiler to treat the pointer access as unaligned.  */
struct ulw
{
  unsigned b;
} __attribute__ ((packed));

#define _ULW(__x) ((struct ulw *) ((char *)(&__x)))->b;
#endif

/* This union assumes that small structures can be in registers.  If
   not, then memory accesses will be done - not optimal, but ok.  */
typedef union
{
  unsigned v;
  bits_t b;
} bitfields_t;

#ifndef detect_zero
/* __mips_dsp, __mips_dspr2, and __mips64 are predefined by
   the compiler, based on command line options.  */
#if (__mips_dsp || __mips_dspr2) && !__mips64
#define __mips_using_dsp 1

/* DSP 4-lane (8 unsigned bits per line) subtract and saturate
 * Intrinsic operation. How this works:
 *     Given a 4-byte string of "ABC\0", subtract this as
 *     an unsigned integer from 0x01010101:
 *	   0x01010101
 *       - 0x41424300
 *        -----------
 (         0xbfbebe01 <-- answer without saturation
 *	   0x00000001 <-- answer with saturation
 * When this 4-lane vector is treated as an unsigned int value,
 * a non-zero answer indicates the presence of a zero in the
 * original 4-byte argument.  */

typedef signed char v4i8 __attribute__ ((vector_size (4)));

#define detect_zero(__x,__y,__01s,__80s)\
       ((unsigned) __builtin_mips_subu_s_qb((v4i8) __01s,(v4i8) __x))

    /* sets all 4 lanes to requested byte.  */
#define set_byte_lanes(__x) ((unsigned) __builtin_mips_repl_qb(__x))

    /* sets all 4 lanes to 0x01.  */
#define def_and_set_01(__x) unsigned __x = (unsigned) __builtin_mips_repl_qb(0x01)

    /* sets all 4 lanes to 0x80. Not needed when subu_s.qb used. */
#define def_and_set_80(__x) /* do nothing */

#else
    /* this version, originally published in the 80's, uses
       a reverse-carry-set like determination of the zero byte.
       The steps are, for __x = 0x31ff0001:
       __x - _01s = 0x30fdff00
       ~__x = 0xce00fffe
       ((__x - _01s) & ~__x) = 0x0000ff00
       x & _80s = 0x00008000 <- byte 3 was zero
       Some implementaions naively assume that characters are
       always 7-bit unsigned ASCII. With that assumption, the
       "& ~x" is usually discarded. Since character strings
       are 8-bit, the and is needed to catch the case of
       a false positive when the byte is 0x80. */

#define detect_zero(__x,__y,_01s,_80s)\
	((unsigned) (((__x) - _01s) & ~(__x)) & _80s)

#if __mips64
#define def_and_set_80(__x) unsigned __x =  0x8080808080808080ul
#define def_and_set_01(__x)  unsigned __x = 0x0101010101010101ul
#else
#define def_and_set_80(__x) unsigned __x = 0x80808080ul
#define def_and_set_01(__x) unsigned __x = 0x01010101ul
#endif

#endif
#endif

/* dealing with 'void *' conversions without using extra variables. */
#define get_byte(__x,__idx) (((unsigned char *) (__x))[__idx])
#define set_byte(__x,__idx,__fill) ((unsigned char *) (__x))[__idx] = (__fill)
#define get_word(__x,__idx) (((unsigned *) (__x))[__idx])
#define set_word(__x,__idx,__fill) ((unsigned *) (__x))[__idx] = (__fill)
#define inc_ptr_as(__type,__x,__inc) __x = (void *) (((__type) __x) + (__inc))
#define cvt_ptr_to(__type,__x) ((__type) (__x))

#endif
