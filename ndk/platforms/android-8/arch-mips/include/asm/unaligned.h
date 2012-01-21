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
#ifndef __ASM_GENERIC_UNALIGNED_H
#define __ASM_GENERIC_UNALIGNED_H

#include <linux/compiler.h>

#define get_unaligned(ptr)  ({   struct __packed {   typeof(*(ptr)) __v;   } *__p = (void *) (ptr);   __p->__v;  })

#define put_unaligned(val, ptr)  do {   struct __packed {   typeof(*(ptr)) __v;   } *__p = (void *) (ptr);   __p->__v = (val);  } while(0)

#endif
