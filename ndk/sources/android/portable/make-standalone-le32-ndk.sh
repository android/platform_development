#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Create a standalone toolchain package for Android.

NDK_DIR=`dirname $0`
NDK_DIR=`dirname $NDK_DIR`
NDK_DIR=`dirname $NDK_DIR`
NDK_DIR=`dirname $NDK_DIR`

NDK_BUILDTOOLS_PATH=$NDK_DIR/build/tools

. $NDK_BUILDTOOLS_PATH/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Generate a clang/llvm toolchain installation that includes
a working sysroot. The result is something that can more easily be
used as a standalone cross-compiler, e.g. to run configure and
make scripts."

force_32bit_binaries

register_var_option "--ndk-dir=<path>" NDK_DIR "Take source files from NDK at <path>"

SYSTEM=$HOST_TAG
register_var_option "--system=<name>" SYSTEM "Specify host system"

PACKAGE_DIR=/tmp/ndk-$USER
register_var_option "--package-dir=<path>" PACKAGE_DIR "Place package file in <path>"

INSTALL_DIR=
register_var_option "--install-dir=<path>" INSTALL_DIR "Don't create package, install files to <path> instead."

extract_parameters "$@"

# Check NDK_DIR
if [ ! -d "$NDK_DIR/build/core" ] ; then
    echo "Invalid source NDK directory: $NDK_DIR"
    echo "Please use --ndk-dir=<path> to specify the path of an installed NDK."
    exit 1
fi

ARCH=arm
TOOLCHAIN_NAME=llvm-3.1
PLATFORM=android-14

# Check toolchain name
TOOLCHAIN_PATH="$NDK_DIR/toolchains/$TOOLCHAIN_NAME"
if [ ! -d "$TOOLCHAIN_PATH" ] ; then
    echo "Invalid toolchain name: $TOOLCHAIN_NAME"
    echo "Please use --toolchain=<name> with the name of a toolchain supported by the source NDK."
    echo "Try one of: " `(cd "$NDK_DIR/toolchains" && ls)`
    exit 1
fi

# Check that there are any platform files for it!
(cd $NDK_DIR/platforms && ls -d */arch-${ARCH} >/dev/null 2>&1 )
if [ $? != 0 ] ; then
    echo "Platform $PLATFORM doesn't have any files for this architecture: $ARCH"
    echo "Either use --platform=<name> or --toolchain=<name> to select a different"
    echo "platform or arch-dependent toolchain name (respectively)!"
    exit 1
fi

# Compute source sysroot
SRC_SYSROOT="$NDK_DIR/platforms/$PLATFORM/arch-$ARCH"
if [ ! -d "$SRC_SYSROOT" ] ; then
    echo "No platform files ($PLATFORM) for this architecture: $ARCH"
    exit 1
fi

# Check that we have any prebuilts LLVM toolchain here
if [ ! -d "$TOOLCHAIN_PATH/prebuilt" ] ; then
    echo "Toolchain is missing prebuilt files: $TOOLCHAIN_NAME"
    echo "You must point to a valid NDK release package!"
    exit 1
fi

if [ ! -d "$TOOLCHAIN_PATH/prebuilt/$SYSTEM" ] ; then
    echo "Host system '$SYSTEM' is not supported by the source NDK!"
    echo "Try --system=<name> with one of: " `(cd $TOOLCHAIN_PATH/prebuilt && ls) | grep -v gdbserver`
    exit 1
fi

TOOLCHAIN_PATH="$TOOLCHAIN_PATH/prebuilt/$SYSTEM"
TOOLCHAIN_LLVM=$TOOLCHAIN_PATH/bin/clang

if [ ! -f "$TOOLCHAIN_LLVM" ] ; then
    echo "Toolchain $TOOLCHAIN_LLVM is missing!"
    exit 1
fi

# Create temporary directory
TMPDIR=$NDK_TMPDIR/standalone/$TOOLCHAIN_NAME

# Copy the clang/llvm toolchain prebuilt binaries
run copy_directory "$TOOLCHAIN_PATH" "$TMPDIR"

# Create scripts for predefined flags

LLVM_VERSION=3.1
LLVM_VERSION_WITHOUT_DOT=$(echo "$LLVM_VERSION" | sed -e "s!\.!!")

mv "$TMPDIR/bin/clang" "$TMPDIR/bin/clang$LLVM_VERSION_WITHOUT_DOT"
rm "$TMPDIR/bin/clang++"
ln -s "clang$LLVM_VERSION_WITHOUT_DOT" "$TMPDIR/bin/clang++$LLVM_VERSION_WITHOUT_DOT"

cat > "$TMPDIR/bin/clang" <<EOF
\`dirname \$0\`/clang$LLVM_VERSION_WITHOUT_DOT -target le32-none-ndk -emit-llvm "\$@"
EOF

cat > "$TMPDIR/bin/clang++" <<EOF
\`dirname \$0\`/clang++$LLVM_VERSION_WITHOUT_DOT -emit-llvm -target le32-none-ndk "\$@"
EOF
chmod 0755 "$TMPDIR/bin/clang" "$TMPDIR/bin/clang++"

dump "Copying sysroot headers"
# Copy the sysroot under $TMPDIR/sysroot. The toolchain was built to
# expect the sysroot files to be placed there!
run copy_directory_nolinks "$SRC_SYSROOT/usr/include" "$TMPDIR/sysroot/usr/include"

dump "Patch portable headers"
PATCH=`dirname $0`
PATCH=`cd $PATCH && pwd`
PATCH=$PATCH/`basename $0`
# rememeber to change +N if you modify this script
cd $TMPDIR && tail -n +160 $PATCH | patch -p0

# Install or Package
if [ -n "$INSTALL_DIR" ] ; then
    dump "Copying files to: $INSTALL_DIR"
    run copy_directory "$TMPDIR" "$INSTALL_DIR"
else
    PACKAGE_FILE="$PACKAGE_DIR/$TOOLCHAIN_NAME.tar.bz2"
    dump "Creating package file: $PACKAGE_FILE"
    pack_archive "$PACKAGE_FILE" "`dirname $TMPDIR`" "$TOOLCHAIN_NAME"
    fail_panic "Could not create tarball from $TMPDIR"
fi

dump "Cleaning up..."
run rm -rf $TMPDIR

dump "Done."
exit 0

###################### portable headers ######################
diff -rupN sysroot-orig/usr/include/asm/byteorder.h sysroot/usr/include/asm/byteorder.h
--- sysroot-orig/usr/include/asm/byteorder.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/byteorder.h	2012-09-10 14:44:44.000000000 +0800
@@ -19,12 +19,6 @@ static inline __attribute_const__ __u32 
 {
  __u32 t;
 
-#ifndef __thumb__
- if (!__builtin_constant_p(x)) {
-
- __asm__ ("eor\t%0, %1, %1, ror #16" : "=r" (t) : "r" (x));
- } else
-#endif
  t = x ^ ((x << 16) | (x >> 16));
 
  x = (x << 24) | (x >> 8);
diff -rupN sysroot-orig/usr/include/asm/cacheflush.h sysroot/usr/include/asm/cacheflush.h
--- sysroot-orig/usr/include/asm/cacheflush.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/cacheflush.h	2012-09-10 14:44:44.000000000 +0800
@@ -15,7 +15,6 @@
 #include <linux/sched.h>
 #include <linux/mm.h>
 
-#include <asm/glue.h>
 #include <asm/shmparam.h>
 
 #define CACHE_COLOUR(vaddr) ((vaddr & (SHMLBA - 1)) >> PAGE_SHIFT)
diff -rupN sysroot-orig/usr/include/asm/div64.h sysroot/usr/include/asm/div64.h
--- sysroot-orig/usr/include/asm/div64.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/div64.h	2012-09-10 14:44:44.000000000 +0800
@@ -14,14 +14,4 @@
 
 #include <asm/system.h>
 
-#ifdef __ARMEB__
-#define __xh "r0"
-#define __xl "r1"
-#else
-#define __xl "r0"
-#define __xh "r1"
-#endif
-
-#define do_div(n,base)  ({   register unsigned int __base asm("r4") = base;   register unsigned long long __n asm("r0") = n;   register unsigned long long __res asm("r2");   register unsigned int __rem asm(__xh);   asm( __asmeq("%0", __xh)   __asmeq("%1", "r2")   __asmeq("%2", "r0")   __asmeq("%3", "r4")   "bl	__do_div64"   : "=r" (__rem), "=r" (__res)   : "r" (__n), "r" (__base)   : "ip", "lr", "cc");   n = __res;   __rem;  })
-
 #endif
diff -rupN sysroot-orig/usr/include/asm/dma.h sysroot/usr/include/asm/dma.h
--- sysroot-orig/usr/include/asm/dma.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/dma.h	2012-09-10 14:44:44.000000000 +0800
@@ -17,7 +17,6 @@ typedef unsigned int dmach_t;
 #include <linux/spinlock.h>
 #include <asm/system.h>
 #include <asm/scatterlist.h>
-#include <asm/arch/dma.h>
 
 #ifndef MAX_DMA_ADDRESS
 #define MAX_DMA_ADDRESS 0xffffffff
diff -rupN sysroot-orig/usr/include/asm/elf.h sysroot/usr/include/asm/elf.h
--- sysroot-orig/usr/include/asm/elf.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/elf.h	2012-09-10 14:44:44.000000000 +0800
@@ -14,45 +14,24 @@
 
 #include <asm/ptrace.h>
 #include <asm/user.h>
-#ifdef __KERNEL
-#include <asm/procinfo.h>
-#endif
 
 typedef unsigned long elf_greg_t;
 typedef unsigned long elf_freg_t[3];
 
-#define EM_ARM 40
-#define EF_ARM_APCS26 0x08
-#define EF_ARM_SOFT_FLOAT 0x200
-#define EF_ARM_EABI_MASK 0xFF000000
-
-#define R_ARM_NONE 0
-#define R_ARM_PC24 1
-#define R_ARM_ABS32 2
-#define R_ARM_CALL 28
-#define R_ARM_JUMP24 29
 
 #define ELF_NGREG (sizeof (struct pt_regs) / sizeof(elf_greg_t))
 typedef elf_greg_t elf_gregset_t[ELF_NGREG];
 
 typedef struct user_fp elf_fpregset_t;
 
-#define elf_check_arch(x) ( ((x)->e_machine == EM_ARM) && (ELF_PROC_OK((x))) )
-
 #define ELF_CLASS ELFCLASS32
-#ifdef __ARMEB__
-#define ELF_DATA ELFDATA2MSB
-#else
 #define ELF_DATA ELFDATA2LSB
-#endif
-#define ELF_ARCH EM_ARM
 
 #define USE_ELF_CORE_DUMP
 #define ELF_EXEC_PAGESIZE 4096
 
 #define ELF_ET_DYN_BASE (2 * TASK_SIZE / 3)
 
-#define ELF_PLAT_INIT(_r, load_addr) (_r)->ARM_r0 = 0
 
 #define ELF_HWCAP (elf_hwcap)
 
diff -rupN sysroot-orig/usr/include/asm/hw_irq.h sysroot/usr/include/asm/hw_irq.h
--- sysroot-orig/usr/include/asm/hw_irq.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/hw_irq.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,6 +12,5 @@
 #ifndef _ARCH_ARM_HW_IRQ_H
 #define _ARCH_ARM_HW_IRQ_H
 
-#include <asm/mach/irq.h>
 
 #endif
diff -rupN sysroot-orig/usr/include/asm/irq.h sysroot/usr/include/asm/irq.h
--- sysroot-orig/usr/include/asm/irq.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/irq.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,7 +12,6 @@
 #ifndef __ASM_ARM_IRQ_H
 #define __ASM_ARM_IRQ_H
 
-#include <asm/arch/irqs.h>
 
 #ifndef irq_canonicalize
 #define irq_canonicalize(i) (i)
diff -rupN sysroot-orig/usr/include/asm/locks.h sysroot/usr/include/asm/locks.h
--- sysroot-orig/usr/include/asm/locks.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/locks.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,44 +12,7 @@
 #ifndef __ASM_PROC_LOCKS_H
 #define __ASM_PROC_LOCKS_H
 
-#if __LINUX_ARM_ARCH__ >= 6
-
-#define __down_op(ptr,fail)   ({   __asm__ __volatile__(   "@ down_op\n"  "1:	ldrex	lr, [%0]\n"  "	sub	lr, lr, %1\n"  "	strex	ip, lr, [%0]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	teq	lr, #0\n"  "	movmi	ip, %0\n"  "	blmi	" #fail   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   smp_mb();   })
-
-#define __down_op_ret(ptr,fail)   ({   unsigned int ret;   __asm__ __volatile__(   "@ down_op_ret\n"  "1:	ldrex	lr, [%1]\n"  "	sub	lr, lr, %2\n"  "	strex	ip, lr, [%1]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	teq	lr, #0\n"  "	movmi	ip, %1\n"  "	movpl	ip, #0\n"  "	blmi	" #fail "\n"  "	mov	%0, ip"   : "=&r" (ret)   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   smp_mb();   ret;   })
-
-#define __up_op(ptr,wake)   ({   smp_mb();   __asm__ __volatile__(   "@ up_op\n"  "1:	ldrex	lr, [%0]\n"  "	add	lr, lr, %1\n"  "	strex	ip, lr, [%0]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	cmp	lr, #0\n"  "	movle	ip, %0\n"  "	blle	" #wake   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   })
-
 #define RW_LOCK_BIAS 0x01000000
 #define RW_LOCK_BIAS_STR "0x01000000"
 
-#define __down_op_write(ptr,fail)   ({   __asm__ __volatile__(   "@ down_op_write\n"  "1:	ldrex	lr, [%0]\n"  "	sub	lr, lr, %1\n"  "	strex	ip, lr, [%0]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	teq	lr, #0\n"  "	movne	ip, %0\n"  "	blne	" #fail   :   : "r" (ptr), "I" (RW_LOCK_BIAS)   : "ip", "lr", "cc");   smp_mb();   })
-
-#define __up_op_write(ptr,wake)   ({   smp_mb();   __asm__ __volatile__(   "@ up_op_write\n"  "1:	ldrex	lr, [%0]\n"  "	adds	lr, lr, %1\n"  "	strex	ip, lr, [%0]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	movcs	ip, %0\n"  "	blcs	" #wake   :   : "r" (ptr), "I" (RW_LOCK_BIAS)   : "ip", "lr", "cc");   })
-
-#define __down_op_read(ptr,fail)   __down_op(ptr, fail)
-
-#define __up_op_read(ptr,wake)   ({   smp_mb();   __asm__ __volatile__(   "@ up_op_read\n"  "1:	ldrex	lr, [%0]\n"  "	add	lr, lr, %1\n"  "	strex	ip, lr, [%0]\n"  "	teq	ip, #0\n"  "	bne	1b\n"  "	teq	lr, #0\n"  "	moveq	ip, %0\n"  "	bleq	" #wake   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   })
-
-#else
-
-#define __down_op(ptr,fail)   ({   __asm__ __volatile__(   "@ down_op\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%0]\n"  "	subs	lr, lr, %1\n"  "	str	lr, [%0]\n"  "	msr	cpsr_c, ip\n"  "	movmi	ip, %0\n"  "	blmi	" #fail   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   smp_mb();   })
-
-#define __down_op_ret(ptr,fail)   ({   unsigned int ret;   __asm__ __volatile__(   "@ down_op_ret\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%1]\n"  "	subs	lr, lr, %2\n"  "	str	lr, [%1]\n"  "	msr	cpsr_c, ip\n"  "	movmi	ip, %1\n"  "	movpl	ip, #0\n"  "	blmi	" #fail "\n"  "	mov	%0, ip"   : "=&r" (ret)   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   smp_mb();   ret;   })
-
-#define __up_op(ptr,wake)   ({   smp_mb();   __asm__ __volatile__(   "@ up_op\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%0]\n"  "	adds	lr, lr, %1\n"  "	str	lr, [%0]\n"  "	msr	cpsr_c, ip\n"  "	movle	ip, %0\n"  "	blle	" #wake   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   })
-
-#define RW_LOCK_BIAS 0x01000000
-#define RW_LOCK_BIAS_STR "0x01000000"
-
-#define __down_op_write(ptr,fail)   ({   __asm__ __volatile__(   "@ down_op_write\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%0]\n"  "	subs	lr, lr, %1\n"  "	str	lr, [%0]\n"  "	msr	cpsr_c, ip\n"  "	movne	ip, %0\n"  "	blne	" #fail   :   : "r" (ptr), "I" (RW_LOCK_BIAS)   : "ip", "lr", "cc");   smp_mb();   })
-
-#define __up_op_write(ptr,wake)   ({   __asm__ __volatile__(   "@ up_op_write\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%0]\n"  "	adds	lr, lr, %1\n"  "	str	lr, [%0]\n"  "	msr	cpsr_c, ip\n"  "	movcs	ip, %0\n"  "	blcs	" #wake   :   : "r" (ptr), "I" (RW_LOCK_BIAS)   : "ip", "lr", "cc");   smp_mb();   })
-
-#define __down_op_read(ptr,fail)   __down_op(ptr, fail)
-
-#define __up_op_read(ptr,wake)   ({   smp_mb();   __asm__ __volatile__(   "@ up_op_read\n"  "	mrs	ip, cpsr\n"  "	orr	lr, ip, #128\n"  "	msr	cpsr_c, lr\n"  "	ldr	lr, [%0]\n"  "	adds	lr, lr, %1\n"  "	str	lr, [%0]\n"  "	msr	cpsr_c, ip\n"  "	moveq	ip, %0\n"  "	bleq	" #wake   :   : "r" (ptr), "I" (1)   : "ip", "lr", "cc");   })
-
-#endif
-
 #endif
diff -rupN sysroot-orig/usr/include/asm/mc146818rtc.h sysroot/usr/include/asm/mc146818rtc.h
--- sysroot-orig/usr/include/asm/mc146818rtc.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/mc146818rtc.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,12 +12,11 @@
 #ifndef _ASM_MC146818RTC_H
 #define _ASM_MC146818RTC_H
 
-#include <asm/arch/irqs.h>
 #include <asm/io.h>
 
 #ifndef RTC_PORT
 #define RTC_PORT(x) (0x70 + (x))
-#define RTC_ALWAYS_BCD 1  
+#define RTC_ALWAYS_BCD 1
 #endif
 
 #define CMOS_READ(addr) ({  outb_p((addr),RTC_PORT(0));  inb_p(RTC_PORT(1));  })
diff -rupN sysroot-orig/usr/include/asm/memory.h sysroot/usr/include/asm/memory.h
--- sysroot-orig/usr/include/asm/memory.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/memory.h	2012-09-10 14:44:44.000000000 +0800
@@ -19,7 +19,6 @@
 #endif
 
 #include <linux/compiler.h>
-#include <asm/arch/memory.h>
 #include <asm/sizes.h>
 
 #ifndef TASK_SIZE
diff -rupN sysroot-orig/usr/include/asm/module.h sysroot/usr/include/asm/module.h
--- sysroot-orig/usr/include/asm/module.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/module.h	2012-09-10 14:44:44.000000000 +0800
@@ -21,6 +21,5 @@ struct mod_arch_specific
 #define Elf_Sym Elf32_Sym
 #define Elf_Ehdr Elf32_Ehdr
 
-#define MODULE_ARCH_VERMAGIC "ARMv" __stringify(__LINUX_ARM_ARCH__) " "
 
 #endif
diff -rupN sysroot-orig/usr/include/asm/pgalloc.h sysroot/usr/include/asm/pgalloc.h
--- sysroot-orig/usr/include/asm/pgalloc.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/pgalloc.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,8 +12,6 @@
 #ifndef _ASMARM_PGALLOC_H
 #define _ASMARM_PGALLOC_H
 
-#include <asm/domain.h>
-#include <asm/pgtable-hwdef.h>
 #include <asm/processor.h>
 #include <asm/cacheflush.h>
 #include <asm/tlbflush.h>
diff -rupN sysroot-orig/usr/include/asm/pgtable.h sysroot/usr/include/asm/pgtable.h
--- sysroot-orig/usr/include/asm/pgtable.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/pgtable.h	2012-09-10 14:44:44.000000000 +0800
@@ -13,7 +13,6 @@
 #define _ASMARM_PGTABLE_H
 
 #include <asm-generic/4level-fixup.h>
-#include <asm/proc-fns.h>
 
 #include "pgtable-nommu.h"
 
diff -rupN sysroot-orig/usr/include/asm/ptrace.h sysroot/usr/include/asm/ptrace.h
--- sysroot-orig/usr/include/asm/ptrace.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/ptrace.h	2012-09-10 14:44:44.000000000 +0800
@@ -56,15 +56,15 @@
 #define PSR_N_BIT 0x80000000
 #define PCMASK 0
 
-#define PSR_f 0xff000000  
-#define PSR_s 0x00ff0000  
-#define PSR_x 0x0000ff00  
-#define PSR_c 0x000000ff  
+#define PSR_f 0xff000000
+#define PSR_s 0x00ff0000
+#define PSR_x 0x0000ff00
+#define PSR_c 0x000000ff
 
 #ifndef __ASSEMBLY__
 
 struct pt_regs {
- long uregs[18];
+ long uregs[21];   //Increase array size for ARM/x86 compatibility
 };
 
 #define ARM_cpsr uregs[16]
@@ -86,6 +86,28 @@ struct pt_regs {
 #define ARM_r0 uregs[0]
 #define ARM_ORIG_r0 uregs[17]
 
+#define x86_r15 uregs[0]
+#define x86_r14 uregs[1]
+#define x86_r13 uregs[2]
+#define x86_r12 uregs[3]
+#define x86_rbp uregs[4]
+#define x86_rbx uregs[5]
+#define x86_r11 uregs[6]
+#define x86_r10 uregs[7]
+#define x86_r9 uregs[8]
+#define x86_r8 uregs[9]
+#define x86_rax uregs[10]
+#define x86_rcx uregs[11]
+#define x86_rdx uregs[12]
+#define x86_rsi uregs[13]
+#define x86_rdi uregs[14]
+#define x86_orig_rax uregs[15]
+#define x86_rip uregs[16]
+#define x86_cs uregs[17]
+#define x86_eflags uregs[18]
+#define x86_rsp uregs[19]
+#define x86_ss uregs[20]
+
 #define pc_pointer(v)   ((v) & ~PCMASK)
 
 #define instruction_pointer(regs)   (pc_pointer((regs)->ARM_pc))
diff -rupN sysroot-orig/usr/include/asm/sigcontext.h sysroot/usr/include/asm/sigcontext.h
--- sysroot-orig/usr/include/asm/sigcontext.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/sigcontext.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,28 +12,4 @@
 #ifndef _ASMARM_SIGCONTEXT_H
 #define _ASMARM_SIGCONTEXT_H
 
-struct sigcontext {
- unsigned long trap_no;
- unsigned long error_code;
- unsigned long oldmask;
- unsigned long arm_r0;
- unsigned long arm_r1;
- unsigned long arm_r2;
- unsigned long arm_r3;
- unsigned long arm_r4;
- unsigned long arm_r5;
- unsigned long arm_r6;
- unsigned long arm_r7;
- unsigned long arm_r8;
- unsigned long arm_r9;
- unsigned long arm_r10;
- unsigned long arm_fp;
- unsigned long arm_ip;
- unsigned long arm_sp;
- unsigned long arm_lr;
- unsigned long arm_pc;
- unsigned long arm_cpsr;
- unsigned long fault_address;
-};
-
 #endif
diff -rupN sysroot-orig/usr/include/asm/socket.h sysroot/usr/include/asm/socket.h
--- sysroot-orig/usr/include/asm/socket.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/socket.h	2012-08-23 18:50:57.000000000 +0800
@@ -7,73 +7,55 @@
  ***   structures, and macros generated from the original header, and thus,
  ***   contains no copyrightable information.
  ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
  ****************************************************************************
  ****************************************************************************/
 #ifndef _ASMARM_SOCKET_H
 #define _ASMARM_SOCKET_H
+
 #include <asm/sockios.h>
+
 #define SOL_SOCKET 1
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
+
 #define SO_DEBUG 1
 #define SO_REUSEADDR 2
 #define SO_TYPE 3
 #define SO_ERROR 4
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define SO_DONTROUTE 5
 #define SO_BROADCAST 6
 #define SO_SNDBUF 7
 #define SO_RCVBUF 8
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define SO_SNDBUFFORCE 32
 #define SO_RCVBUFFORCE 33
 #define SO_KEEPALIVE 9
 #define SO_OOBINLINE 10
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define SO_NO_CHECK 11
 #define SO_PRIORITY 12
 #define SO_LINGER 13
 #define SO_BSDCOMPAT 14
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
+
 #define SO_PASSCRED 16
 #define SO_PEERCRED 17
 #define SO_RCVLOWAT 18
 #define SO_SNDLOWAT 19
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define SO_RCVTIMEO 20
 #define SO_SNDTIMEO 21
+
 #define SO_SECURITY_AUTHENTICATION 22
 #define SO_SECURITY_ENCRYPTION_TRANSPORT 23
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define SO_SECURITY_ENCRYPTION_NETWORK 24
+
 #define SO_BINDTODEVICE 25
+
 #define SO_ATTACH_FILTER 26
 #define SO_DETACH_FILTER 27
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
+
 #define SO_PEERNAME 28
 #define SO_TIMESTAMP 29
 #define SCM_TIMESTAMP SO_TIMESTAMP
+
 #define SO_ACCEPTCONN 30
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
+
 #define SO_PEERSEC 31
 #define SO_PASSSEC 34
-#define SO_TIMESTAMPNS 35
-#define SCM_TIMESTAMPNS SO_TIMESTAMPNS
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_MARK 36
-#define SO_TIMESTAMPING 37
-#define SCM_TIMESTAMPING SO_TIMESTAMPING
-#define SO_PROTOCOL 38
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_DOMAIN 39
-#define SO_RXQ_OVFL 40
-#define SO_WIFI_STATUS 41
-#define SCM_WIFI_STATUS SO_WIFI_STATUS
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
+
 #endif
diff -rupN sysroot-orig/usr/include/asm/stat.h sysroot/usr/include/asm/stat.h
--- sysroot-orig/usr/include/asm/stat.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/stat.h	2012-09-10 14:44:44.000000000 +0800
@@ -26,26 +26,16 @@ struct __old_kernel_stat {
  unsigned long st_ctime;
 };
 
-#define STAT_HAVE_NSEC 
+#define STAT_HAVE_NSEC
 
 struct stat {
-#ifdef __ARMEB__
- unsigned short st_dev;
- unsigned short __pad1;
-#else
  unsigned long st_dev;
-#endif
  unsigned long st_ino;
  unsigned short st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
-#ifdef __ARMEB__
- unsigned short st_rdev;
- unsigned short __pad2;
-#else
  unsigned long st_rdev;
-#endif
  unsigned long st_size;
  unsigned long st_blksize;
  unsigned long st_blocks;
diff -rupN sysroot-orig/usr/include/asm/timex.h sysroot/usr/include/asm/timex.h
--- sysroot-orig/usr/include/asm/timex.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/timex.h	2012-09-10 14:44:44.000000000 +0800
@@ -12,7 +12,6 @@
 #ifndef _ASMARM_TIMEX_H
 #define _ASMARM_TIMEX_H
 
-#include <asm/arch/timex.h>
 
 typedef unsigned long cycles_t;
 
diff -rupN sysroot-orig/usr/include/asm/uaccess.h sysroot/usr/include/asm/uaccess.h
--- sysroot-orig/usr/include/asm/uaccess.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/uaccess.h	2012-09-10 14:44:44.000000000 +0800
@@ -15,7 +15,6 @@
 #include <linux/sched.h>
 #include <asm/errno.h>
 #include <asm/memory.h>
-#include <asm/domain.h>
 #include <asm/system.h>
 
 #define VERIFY_READ 0
@@ -36,37 +35,9 @@ struct exception_table_entry
 #define __range_ok(addr,size) (0)
 #define get_fs() (KERNEL_DS)
 
-#define get_user(x,p) __get_user(x,p)
-#define put_user(x,p) __put_user(x,p)
+#define get_user(x,p)
+#define put_user(x,p)
 #define access_ok(type,addr,size) (__range_ok(addr,size) == 0)
-#define __get_user(x,ptr)  ({   long __gu_err = 0;   __get_user_err((x),(ptr),__gu_err);   __gu_err;  })
-#define __get_user_error(x,ptr,err)  ({   __get_user_err((x),(ptr),err);   (void) 0;  })
-#define __get_user_err(x,ptr,err)  do {   unsigned long __gu_addr = (unsigned long)(ptr);   unsigned long __gu_val;   __chk_user_ptr(ptr);   switch (sizeof(*(ptr))) {   case 1: __get_user_asm_byte(__gu_val,__gu_addr,err); break;   case 2: __get_user_asm_half(__gu_val,__gu_addr,err); break;   case 4: __get_user_asm_word(__gu_val,__gu_addr,err); break;   default: (__gu_val) = __get_user_bad();   }   (x) = (__typeof__(*(ptr)))__gu_val;  } while (0)
-#define __get_user_asm_byte(x,addr,err)   __asm__ __volatile__(   "1:	ldrbt	%1,[%2],#0\n"   "2:\n"   "	.section .fixup,\"ax\"\n"   "	.align	2\n"   "3:	mov	%0, %3\n"   "	mov	%1, #0\n"   "	b	2b\n"   "	.previous\n"   "	.section __ex_table,\"a\"\n"   "	.align	3\n"   "	.long	1b, 3b\n"   "	.previous"   : "+r" (err), "=&r" (x)   : "r" (addr), "i" (-EFAULT)   : "cc")
-#ifndef __ARMEB__
-#define __get_user_asm_half(x,__gu_addr,err)  ({   unsigned long __b1, __b2;   __get_user_asm_byte(__b1, __gu_addr, err);   __get_user_asm_byte(__b2, __gu_addr + 1, err);   (x) = __b1 | (__b2 << 8);  })
-#else
-#define __get_user_asm_half(x,__gu_addr,err)  ({   unsigned long __b1, __b2;   __get_user_asm_byte(__b1, __gu_addr, err);   __get_user_asm_byte(__b2, __gu_addr + 1, err);   (x) = (__b1 << 8) | __b2;  })
-#endif
-#define __get_user_asm_word(x,addr,err)   __asm__ __volatile__(   "1:	ldrt	%1,[%2],#0\n"   "2:\n"   "	.section .fixup,\"ax\"\n"   "	.align	2\n"   "3:	mov	%0, %3\n"   "	mov	%1, #0\n"   "	b	2b\n"   "	.previous\n"   "	.section __ex_table,\"a\"\n"   "	.align	3\n"   "	.long	1b, 3b\n"   "	.previous"   : "+r" (err), "=&r" (x)   : "r" (addr), "i" (-EFAULT)   : "cc")
-#define __put_user(x,ptr)  ({   long __pu_err = 0;   __put_user_err((x),(ptr),__pu_err);   __pu_err;  })
-#define __put_user_error(x,ptr,err)  ({   __put_user_err((x),(ptr),err);   (void) 0;  })
-#define __put_user_err(x,ptr,err)  do {   unsigned long __pu_addr = (unsigned long)(ptr);   __typeof__(*(ptr)) __pu_val = (x);   __chk_user_ptr(ptr);   switch (sizeof(*(ptr))) {   case 1: __put_user_asm_byte(__pu_val,__pu_addr,err); break;   case 2: __put_user_asm_half(__pu_val,__pu_addr,err); break;   case 4: __put_user_asm_word(__pu_val,__pu_addr,err); break;   case 8: __put_user_asm_dword(__pu_val,__pu_addr,err); break;   default: __put_user_bad();   }  } while (0)
-#define __put_user_asm_byte(x,__pu_addr,err)   __asm__ __volatile__(   "1:	strbt	%1,[%2],#0\n"   "2:\n"   "	.section .fixup,\"ax\"\n"   "	.align	2\n"   "3:	mov	%0, %3\n"   "	b	2b\n"   "	.previous\n"   "	.section __ex_table,\"a\"\n"   "	.align	3\n"   "	.long	1b, 3b\n"   "	.previous"   : "+r" (err)   : "r" (x), "r" (__pu_addr), "i" (-EFAULT)   : "cc")
-#ifndef __ARMEB__
-#define __put_user_asm_half(x,__pu_addr,err)  ({   unsigned long __temp = (unsigned long)(x);   __put_user_asm_byte(__temp, __pu_addr, err);   __put_user_asm_byte(__temp >> 8, __pu_addr + 1, err);  })
-#else
-#define __put_user_asm_half(x,__pu_addr,err)  ({   unsigned long __temp = (unsigned long)(x);   __put_user_asm_byte(__temp >> 8, __pu_addr, err);   __put_user_asm_byte(__temp, __pu_addr + 1, err);  })
-#endif
-#define __put_user_asm_word(x,__pu_addr,err)   __asm__ __volatile__(   "1:	strt	%1,[%2],#0\n"   "2:\n"   "	.section .fixup,\"ax\"\n"   "	.align	2\n"   "3:	mov	%0, %3\n"   "	b	2b\n"   "	.previous\n"   "	.section __ex_table,\"a\"\n"   "	.align	3\n"   "	.long	1b, 3b\n"   "	.previous"   : "+r" (err)   : "r" (x), "r" (__pu_addr), "i" (-EFAULT)   : "cc")
-#ifndef __ARMEB__
-#define __reg_oper0 "%R2"
-#define __reg_oper1 "%Q2"
-#else
-#define __reg_oper0 "%Q2"
-#define __reg_oper1 "%R2"
-#endif
-#define __put_user_asm_dword(x,__pu_addr,err)   __asm__ __volatile__(   "1:	strt	" __reg_oper1 ", [%1], #4\n"   "2:	strt	" __reg_oper0 ", [%1], #0\n"   "3:\n"   "	.section .fixup,\"ax\"\n"   "	.align	2\n"   "4:	mov	%0, %3\n"   "	b	3b\n"   "	.previous\n"   "	.section __ex_table,\"a\"\n"   "	.align	3\n"   "	.long	1b, 4b\n"   "	.long	2b, 4b\n"   "	.previous"   : "+r" (err), "+r" (__pu_addr)   : "r" (x), "i" (-EFAULT)   : "cc")
 #define __copy_from_user(to,from,n) (memcpy(to, (void __force *)from, n), 0)
 #define __copy_to_user(to,from,n) (memcpy((void __force *)to, from, n), 0)
 #define __clear_user(addr,n) (memset((void __force *)addr, 0, n), 0)
diff -rupN sysroot-orig/usr/include/asm/unaligned.h sysroot/usr/include/asm/unaligned.h
--- sysroot-orig/usr/include/asm/unaligned.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/unaligned.h	2012-09-10 14:44:44.000000000 +0800
@@ -28,11 +28,6 @@
 
 #define __put_unaligned_le(val,ptr)   ({   switch (sizeof(*(ptr))) {   case 1:   *(ptr) = (val);   break;   case 2: __put_unaligned_2_le((val),(__u8 *)(ptr));   break;   case 4: __put_unaligned_4_le((val),(__u8 *)(ptr));   break;   case 8: __put_unaligned_8_le((val),(__u8 *)(ptr));   break;   default: __bug_unaligned_x(ptr);   break;   }   (void) 0;   })
 #define __put_unaligned_be(val,ptr)   ({   switch (sizeof(*(ptr))) {   case 1:   *(ptr) = (val);   break;   case 2: __put_unaligned_2_be((val),(__u8 *)(ptr));   break;   case 4: __put_unaligned_4_be((val),(__u8 *)(ptr));   break;   case 8: __put_unaligned_8_be((val),(__u8 *)(ptr));   break;   default: __bug_unaligned_x(ptr);   break;   }   (void) 0;   })
-#ifndef __ARMEB__
 #define get_unaligned __get_unaligned_le
 #define put_unaligned __put_unaligned_le
-#else
-#define get_unaligned __get_unaligned_be
-#define put_unaligned __put_unaligned_be
-#endif
 #endif
diff -rupN sysroot-orig/usr/include/asm/unistd.h sysroot/usr/include/asm/unistd.h
--- sysroot-orig/usr/include/asm/unistd.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/unistd.h	2012-10-31 17:31:57.954442000 +0800
@@ -18,13 +18,8 @@
  ****************************************************************************/
 #ifndef __ASM_ARM_UNISTD_H
 #define __ASM_ARM_UNISTD_H
-#define __NR_OABI_SYSCALL_BASE 0x900000
-#if defined(__thumb__) || defined(__ARM_EABI__)
 /* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define __NR_SYSCALL_BASE 0
-#else
-#define __NR_SYSCALL_BASE __NR_OABI_SYSCALL_BASE
-#endif
 /* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 #define __NR_restart_syscall (__NR_SYSCALL_BASE+ 0)
 #define __NR_exit (__NR_SYSCALL_BASE+ 1)
diff -rupN sysroot-orig/usr/include/asm/user.h sysroot/usr/include/asm/user.h
--- sysroot-orig/usr/include/asm/user.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm/user.h	2012-09-10 14:44:44.000000000 +0800
@@ -58,15 +58,4 @@ struct user{
 #define HOST_TEXT_START_ADDR (u.start_code)
 #define HOST_STACK_END_ADDR (u.start_stack + u.u_ssize * NBPG)
 
-struct user_vfp {
- unsigned long long fpregs[32];
- unsigned long fpscr;
-};
-
-struct user_vfp_exc {
- unsigned long fpexc;
- unsigned long fpinst;
- unsigned long fpinst2;
-};
-
 #endif
diff -rupN sysroot-orig/usr/include/asm-generic/mman-common.h sysroot/usr/include/asm-generic/mman-common.h
--- sysroot-orig/usr/include/asm-generic/mman-common.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm-generic/mman-common.h	1970-01-01 08:00:00.000000000 +0800
@@ -1,60 +0,0 @@
-/****************************************************************************
- ****************************************************************************
- ***
- ***   This header was automatically generated from a Linux kernel header
- ***   of the same name, to make information necessary for userspace to
- ***   call into the kernel available to libc.  It contains only constants,
- ***   structures, and macros generated from the original header, and thus,
- ***   contains no copyrightable information.
- ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
- ****************************************************************************
- ****************************************************************************/
-#ifndef __ASM_GENERIC_MMAN_COMMON_H
-#define __ASM_GENERIC_MMAN_COMMON_H
-#define PROT_READ 0x1  
-#define PROT_WRITE 0x2  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define PROT_EXEC 0x4  
-#define PROT_SEM 0x8  
-#define PROT_NONE 0x0  
-#define PROT_GROWSDOWN 0x01000000  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define PROT_GROWSUP 0x02000000  
-#define MAP_SHARED 0x01  
-#define MAP_PRIVATE 0x02  
-#define MAP_TYPE 0x0f  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MAP_FIXED 0x10  
-#define MAP_ANONYMOUS 0x20  
-#define MAP_UNINITIALIZED 0x0  
-#define MS_ASYNC 1  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MS_INVALIDATE 2  
-#define MS_SYNC 4  
-#define MADV_NORMAL 0  
-#define MADV_RANDOM 1  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MADV_SEQUENTIAL 2  
-#define MADV_WILLNEED 3  
-#define MADV_DONTNEED 4  
-#define MADV_REMOVE 9  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MADV_DONTFORK 10  
-#define MADV_DOFORK 11  
-#define MADV_HWPOISON 100  
-#define MADV_SOFT_OFFLINE 101  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MADV_MERGEABLE 12  
-#define MADV_UNMERGEABLE 13  
-#define MADV_HUGEPAGE 14  
-#define MADV_NOHUGEPAGE 15  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MAP_FILE 0
-#endif
diff -rupN sysroot-orig/usr/include/asm-generic/mman.h sysroot/usr/include/asm-generic/mman.h
--- sysroot-orig/usr/include/asm-generic/mman.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm-generic/mman.h	2012-08-23 18:50:57.000000000 +0800
@@ -7,30 +7,40 @@
  ***   structures, and macros generated from the original header, and thus,
  ***   contains no copyrightable information.
  ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
  ****************************************************************************
  ****************************************************************************/
-#ifndef __ASM_GENERIC_MMAN_H
-#define __ASM_GENERIC_MMAN_H
-#include <asm-generic/mman-common.h>
-#define MAP_GROWSDOWN 0x0100  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MAP_DENYWRITE 0x0800  
-#define MAP_EXECUTABLE 0x1000  
-#define MAP_LOCKED 0x2000  
-#define MAP_NORESERVE 0x4000  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MAP_POPULATE 0x8000  
-#define MAP_NONBLOCK 0x10000  
-#define MAP_STACK 0x20000  
-#define MAP_HUGETLB 0x40000  
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define MCL_CURRENT 1  
-#define MCL_FUTURE 2  
+#ifndef _ASM_GENERIC_MMAN_H
+#define _ASM_GENERIC_MMAN_H
+
+#define PROT_READ 0x1  
+#define PROT_WRITE 0x2  
+#define PROT_EXEC 0x4  
+#define PROT_SEM 0x8  
+#define PROT_NONE 0x0  
+#define PROT_GROWSDOWN 0x01000000  
+#define PROT_GROWSUP 0x02000000  
+
+#define MAP_SHARED 0x01  
+#define MAP_PRIVATE 0x02  
+#define MAP_TYPE 0x0f  
+#define MAP_FIXED 0x10  
+#define MAP_ANONYMOUS 0x20  
+
+#define MS_ASYNC 1  
+#define MS_INVALIDATE 2  
+#define MS_SYNC 4  
+
+#define MADV_NORMAL 0  
+#define MADV_RANDOM 1  
+#define MADV_SEQUENTIAL 2  
+#define MADV_WILLNEED 3  
+#define MADV_DONTNEED 4  
+
+#define MADV_REMOVE 9  
+#define MADV_DONTFORK 10  
+#define MADV_DOFORK 11  
+
+#define MAP_ANON MAP_ANONYMOUS
+#define MAP_FILE 0
+
 #endif
diff -rupN sysroot-orig/usr/include/asm-generic/pgtable-nopmd.h sysroot/usr/include/asm-generic/pgtable-nopmd.h
--- sysroot-orig/usr/include/asm-generic/pgtable-nopmd.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm-generic/pgtable-nopmd.h	1970-01-01 08:00:00.000000000 +0800
@@ -1,48 +0,0 @@
-/****************************************************************************
- ****************************************************************************
- ***
- ***   This header was automatically generated from a Linux kernel header
- ***   of the same name, to make information necessary for userspace to
- ***   call into the kernel available to libc.  It contains only constants,
- ***   structures, and macros generated from the original header, and thus,
- ***   contains no copyrightable information.
- ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
- ****************************************************************************
- ****************************************************************************/
-#ifndef _PGTABLE_NOPMD_H
-#define _PGTABLE_NOPMD_H
-#ifndef __ASSEMBLY__
-#include <asm-generic/pgtable-nopud.h>
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-struct mm_struct;
-#define __PAGETABLE_PMD_FOLDED
-typedef struct { pud_t pud; } pmd_t;
-#define PMD_SHIFT PUD_SHIFT
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define PTRS_PER_PMD 1
-#define PMD_SIZE (1UL << PMD_SHIFT)
-#define PMD_MASK (~(PMD_SIZE-1))
-#define pmd_ERROR(pmd) (pud_ERROR((pmd).pud))
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define pud_populate(mm, pmd, pte) do { } while (0)
-#define set_pud(pudptr, pudval) set_pmd((pmd_t *)(pudptr), (pmd_t) { pudval })
-#define pmd_val(x) (pud_val((x).pud))
-#define __pmd(x) ((pmd_t) { __pud(x) } )
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define pud_page(pud) (pmd_page((pmd_t){ pud }))
-#define pud_page_vaddr(pud) (pmd_page_vaddr((pmd_t){ pud }))
-#define pmd_alloc_one(mm, address) NULL
-#define __pmd_free_tlb(tlb, x, a) do { } while (0)
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#undef pmd_addr_end
-#define pmd_addr_end(addr, end) (end)
-#endif
-#endif
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
diff -rupN sysroot-orig/usr/include/asm-generic/socket.h sysroot/usr/include/asm-generic/socket.h
--- sysroot-orig/usr/include/asm-generic/socket.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm-generic/socket.h	1970-01-01 08:00:00.000000000 +0800
@@ -1,81 +0,0 @@
-/****************************************************************************
- ****************************************************************************
- ***
- ***   This header was automatically generated from a Linux kernel header
- ***   of the same name, to make information necessary for userspace to
- ***   call into the kernel available to libc.  It contains only constants,
- ***   structures, and macros generated from the original header, and thus,
- ***   contains no copyrightable information.
- ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
- ****************************************************************************
- ****************************************************************************/
-#ifndef __ASM_GENERIC_SOCKET_H
-#define __ASM_GENERIC_SOCKET_H
-#include <asm/sockios.h>
-#define SOL_SOCKET 1
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_DEBUG 1
-#define SO_REUSEADDR 2
-#define SO_TYPE 3
-#define SO_ERROR 4
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_DONTROUTE 5
-#define SO_BROADCAST 6
-#define SO_SNDBUF 7
-#define SO_RCVBUF 8
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_SNDBUFFORCE 32
-#define SO_RCVBUFFORCE 33
-#define SO_KEEPALIVE 9
-#define SO_OOBINLINE 10
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_NO_CHECK 11
-#define SO_PRIORITY 12
-#define SO_LINGER 13
-#define SO_BSDCOMPAT 14
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#ifndef SO_PASSCRED
-#define SO_PASSCRED 16
-#define SO_PEERCRED 17
-#define SO_RCVLOWAT 18
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_SNDLOWAT 19
-#define SO_RCVTIMEO 20
-#define SO_SNDTIMEO 21
-#endif
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_SECURITY_AUTHENTICATION 22
-#define SO_SECURITY_ENCRYPTION_TRANSPORT 23
-#define SO_SECURITY_ENCRYPTION_NETWORK 24
-#define SO_BINDTODEVICE 25
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_ATTACH_FILTER 26
-#define SO_DETACH_FILTER 27
-#define SO_PEERNAME 28
-#define SO_TIMESTAMP 29
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SCM_TIMESTAMP SO_TIMESTAMP
-#define SO_ACCEPTCONN 30
-#define SO_PEERSEC 31
-#define SO_PASSSEC 34
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_TIMESTAMPNS 35
-#define SCM_TIMESTAMPNS SO_TIMESTAMPNS
-#define SO_MARK 36
-#define SO_TIMESTAMPING 37
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SCM_TIMESTAMPING SO_TIMESTAMPING
-#define SO_PROTOCOL 38
-#define SO_DOMAIN 39
-#define SO_RXQ_OVFL 40
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#define SO_WIFI_STATUS 41
-#define SCM_WIFI_STATUS SO_WIFI_STATUS
-#endif
diff -rupN sysroot-orig/usr/include/asm-generic/swab.h sysroot/usr/include/asm-generic/swab.h
--- sysroot-orig/usr/include/asm-generic/swab.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/asm-generic/swab.h	1970-01-01 08:00:00.000000000 +0800
@@ -1,29 +0,0 @@
-/****************************************************************************
- ****************************************************************************
- ***
- ***   This header was automatically generated from a Linux kernel header
- ***   of the same name, to make information necessary for userspace to
- ***   call into the kernel available to libc.  It contains only constants,
- ***   structures, and macros generated from the original header, and thus,
- ***   contains no copyrightable information.
- ***
- ***   To edit the content of this header, modify the corresponding
- ***   source file (e.g. under external/kernel-headers/original/) then
- ***   run bionic/libc/kernel/tools/update_all.py
- ***
- ***   Any manual change here will be lost the next time this script will
- ***   be run. You've been warned!
- ***
- ****************************************************************************
- ****************************************************************************/
-#ifndef _ASM_GENERIC_SWAB_H
-#define _ASM_GENERIC_SWAB_H
-#include <asm/bitsperlong.h>
-#if __BITS_PER_LONG == 32
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#if defined(__GNUC__) && (!defined(__STRICT_ANSI__) || defined(__KERNEL__))
-#define __SWAB_64_THRU_32__
-#endif
-#endif
-/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
-#endif
diff -rupN sysroot-orig/usr/include/limits.h sysroot/usr/include/limits.h
--- sysroot-orig/usr/include/limits.h	2012-10-22 13:52:50.000000000 +0800
+++ sysroot/usr/include/limits.h	2012-08-23 18:50:57.000000000 +0800
@@ -89,22 +89,6 @@
 #include <sys/syslimits.h>
 #endif
 
-/* GLibc compatibility definitions.
-   Note that these are defined by GCC's <limits.h>
-   only when __GNU_LIBRARY__ is defined, i.e. when
-   targetting GLibc. */
-#ifndef LONG_LONG_MIN
-#define LONG_LONG_MIN  LLONG_MIN
-#endif
-
-#ifndef LONG_LONG_MAX
-#define LONG_LONG_MAX  LLONG_MAX
-#endif
-
-#ifndef ULONG_LONG_MAX
-#define ULONG_LONG_MAX  ULLONG_MAX
-#endif
-
 #ifndef PAGESIZE
 #define  PAGESIZE  PAGE_SIZE
 #endif
diff -rupN sysroot-orig/usr/include/linux/icmp.h sysroot/usr/include/linux/icmp.h
--- sysroot-orig/usr/include/linux/icmp.h	2012-10-15 15:56:36.000000000 +0800
+++ sysroot/usr/include/linux/icmp.h	2012-08-23 18:50:57.000000000 +0800
@@ -66,7 +66,7 @@ struct icmphdr {
  } echo;
  __u32 gateway;
  struct {
- __u16 __linux_unused;
+ __u16 __unused_field;
  __u16 mtu;
  } frag;
  } un;
diff -rupN sysroot-orig/usr/include/linux/sysctl.h sysroot/usr/include/linux/sysctl.h
--- sysroot-orig/usr/include/linux/sysctl.h	2012-10-15 15:56:36.000000000 +0800
+++ sysroot/usr/include/linux/sysctl.h	2012-08-23 18:50:57.000000000 +0800
@@ -28,7 +28,7 @@ struct __sysctl_args {
  size_t __user *oldlenp;
  void __user *newval;
  size_t newlen;
- unsigned long __linux_unused[4];
+ unsigned long __unused[4];
 };
 
 enum
diff -rupN sysroot-orig/usr/include/machine/_types.h sysroot/usr/include/machine/_types.h
--- sysroot-orig/usr/include/machine/_types.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/machine/_types.h	2012-09-10 14:44:44.000000000 +0800
@@ -36,16 +36,30 @@
 #define _ARM__TYPES_H_
 
 
-#if !defined(__ARM_EABI__)
 /* the kernel defines size_t as unsigned int, but g++ wants it to be unsigned long */
-#define _SIZE_T
-#define _SSIZE_T
+#ifndef _SIZE_T_DEFINED_
+#define _SIZE_T_DEFINED_
+#  ifdef __ANDROID__
+     typedef unsigned int  size_t;
+#  else
+     typedef unsigned long  size_t;
+#  endif
+#endif
+#ifndef _SSIZE_T_DEFINED_
+#define _SSIZE_T_DEFINED_
+typedef long int          ssize_t;
+#endif
+#ifndef _PTRDIFF_T
 #define _PTRDIFF_T
-typedef unsigned long  size_t;
-typedef long           ssize_t;
-typedef long           ptrdiff_t;
+#  ifdef __ANDROID__
+     typedef int            ptrdiff_t;
+#  else
+     typedef long           ptrdiff_t;
+#  endif
 #endif
 
+//#include <linux/types.h>
+
 /* 7.18.1.1 Exact-width integer types */
 typedef	__signed char		__int8_t;
 typedef	unsigned char		__uint8_t;
@@ -80,7 +94,7 @@ typedef	__uint64_t		__uint_fast64_t;
 
 /* 7.18.1.4 Integer types capable of holding object pointers */
 typedef	int 			__intptr_t;
-typedef	unsigned int 		__uintptr_t;
+typedef	unsigned int 	__uintptr_t;
 
 /* 7.18.1.5 Greatest-width integer types */
 typedef	__int64_t		__intmax_t;
@@ -122,4 +136,4 @@ typedef	void *			__wctype_t;
 #define _BYTE_ORDER _LITTLE_ENDIAN
 #endif
 
-#endif	/* _ARM__TYPES_H_ */
+#endif	/* _MACHINE__TYPES_H_ */
diff -rupN sysroot-orig/usr/include/machine/asm.h sysroot/usr/include/machine/asm.h
--- sysroot-orig/usr/include/machine/asm.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/machine/asm.h	2012-09-10 14:44:44.000000000 +0800
@@ -70,38 +70,15 @@
 #define _ASM_TYPE_FUNCTION	#function
 #define _ASM_TYPE_OBJECT	#object
 #define _ENTRY(x) \
-	.text; _ALIGN_TEXT; .globl x; .type x,_ASM_TYPE_FUNCTION; x: .fnstart
+	.text; _ALIGN_TEXT; .globl x; .type x,_ASM_TYPE_FUNCTION; x:
 
-#define _ASM_SIZE(x)	.size x, .-x;
 
-#define _END(x) \
-	.fnend; \
-	_ASM_SIZE(x)
-
-#ifdef GPROF
-# ifdef __ELF__
-#  define _PROF_PROLOGUE	\
-	mov ip, lr; bl __mcount
-# else
-#  define _PROF_PROLOGUE	\
-	mov ip,lr; bl mcount
-# endif
-#else
 # define _PROF_PROLOGUE
-#endif
 
 #define	ENTRY(y)	_ENTRY(_C_LABEL(y)); _PROF_PROLOGUE
 #define	ENTRY_NP(y)	_ENTRY(_C_LABEL(y))
-#define	END(y)		_END(_C_LABEL(y))
 #define	ASENTRY(y)	_ENTRY(_ASM_LABEL(y)); _PROF_PROLOGUE
 #define	ASENTRY_NP(y)	_ENTRY(_ASM_LABEL(y))
-#define	ASEND(y)	_END(_ASM_LABEL(y))
-
-#ifdef __ELF__
-#define ENTRY_PRIVATE(y)  ENTRY(y); .hidden _C_LABEL(y)
-#else
-#define ENTRY_PRIVATE(y)  ENTRY(y)
-#endif
 
 #define	ASMSTR		.asciz
 
diff -rupN sysroot-orig/usr/include/machine/exec.h sysroot/usr/include/machine/exec.h
--- sysroot-orig/usr/include/machine/exec.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/machine/exec.h	2012-09-10 14:44:44.000000000 +0800
@@ -39,7 +39,6 @@
 
 #define ELF_TARG_CLASS		ELFCLASS32
 #define ELF_TARG_DATA		ELFDATA2LSB
-#define ELF_TARG_MACH		EM_ARM
 
 #define _NLIST_DO_AOUT
 #define _NLIST_DO_ELF
diff -rupN sysroot-orig/usr/include/machine/setjmp.h sysroot/usr/include/machine/setjmp.h
--- sysroot-orig/usr/include/machine/setjmp.h	2012-10-04 20:52:38.000000000 +0800
+++ sysroot/usr/include/machine/setjmp.h	2012-09-10 14:44:44.000000000 +0800
@@ -1,82 +1,88 @@
-/*
- * Copyright (C) 2010 The Android Open Source Project
- * All rights reserved.
- *
- * Redistribution and use in source and binary forms, with or without
- * modification, are permitted provided that the following conditions
- * are met:
- *  * Redistributions of source code must retain the above copyright
- *    notice, this list of conditions and the following disclaimer.
- *  * Redistributions in binary form must reproduce the above copyright
- *    notice, this list of conditions and the following disclaimer in
- *    the documentation and/or other materials provided with the
- *    distribution.
- *
- * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
- * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
- * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
- * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
- * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
- * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
- * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
- * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
- * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
- * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
- * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
- * SUCH DAMAGE.
- */
+/*	$OpenBSD: setjmp.h,v 1.1 2004/02/01 05:09:49 drahn Exp $	*/
+/*	$NetBSD: setjmp.h,v 1.2 2001/08/25 14:45:59 bjh21 Exp $	*/
 
 /*
  * machine/setjmp.h: machine dependent setjmp-related information.
  */
 
-/* _JBLEN is the size of a jmp_buf in longs.
- * Do not modify this value or you will break the ABI !
- *
- * This value comes from the original OpenBSD ARM-specific header
- * that was replaced by this one.
- */
-#define _JBLEN  64
+#ifdef __ELF__
+#define _JBLEN 157      /* size, in longs, of a jmp_buf */
+                        /* For compatible with all architectures, x86:10, arm:64, mips:157 */
+#else
+#define	_JBLEN	29		/* size, in longs, of a jmp_buf */
+#endif
 
-/* According to the ARM AAPCS document, we only need to save
- * the following registers:
- *
- *  Core   r4-r14
- *
- *  VFP    d8-d15  (see section 5.1.2.1)
- *
- *      Registers s16-s31 (d8-d15, q4-q7) must be preserved across subroutine
- *      calls; registers s0-s15 (d0-d7, q0-q3) do not need to be preserved
- *      (and can be used for passing arguments or returning results in standard
- *      procedure-call variants). Registers d16-d31 (q8-q15), if present, do
- *      not need to be preserved.
- *
- *  FPSCR  saved because GLibc does saves it too.
- *
- */
-
-/* The internal structure of a jmp_buf is totally private.
- * Current layout (may change in the future):
- *
- * word   name         description
- * 0      magic        magic number
- * 1      sigmask      signal mask (not used with _setjmp / _longjmp)
- * 2      float_base   base of float registers (d8 to d15)
- * 18     float_state  floating-point status and control register
- * 19     core_base    base of core registers (r4 to r14)
- * 30     reserved     reserved entries (room to grow)
- * 64
- *
- * NOTE: float_base must be at an even word index, since the
- *       FP registers will be loaded/stored with instructions
- *       that expect 8-byte alignment.
+/*
+ * NOTE: The internal structure of a jmp_buf is *PRIVATE*
+ *       This information is provided as there is software
+ *       that fiddles with this with obtain the stack pointer
+ *	 (yes really ! and its commercial !).
+ *
+ * Description of the setjmp buffer
+ *
+ * word  0	magic number	(dependant on creator)
+ *       1 -  3	f4		fp register 4
+ *	 4 -  6	f5		fp register 5
+ *	 7 -  9 f6		fp register 6
+ *	10 - 12	f7		fp register 7
+ *	13	fpsr		fp status register
+ *	14	r4		register 4
+ *	15	r5		register 5
+ *	16	r6		register 6
+ *	17	r7		register 7
+ *	18	r8		register 8
+ *	19	r9		register 9
+ *	20	r10		register 10 (sl)
+ *	21	r11		register 11 (fp)
+ *	22	r12		register 12 (ip)
+ *	23	r13		register 13 (sp)
+ *	24	r14		register 14 (lr)
+ *	25	signal mask	(dependant on magic)
+ *	26	(con't)
+ *	27	(con't)
+ *	28	(con't)
+ *
+ * The magic number number identifies the jmp_buf and
+ * how the buffer was created as well as providing
+ * a sanity check
+ *
+ * A side note I should mention - Please do not tamper
+ * with the floating point fields. While they are
+ * always saved and restored at the moment this cannot
+ * be garenteed especially if the compiler happens
+ * to be generating soft-float code so no fp
+ * registers will be used.
+ *
+ * Whilst this can be seen an encouraging people to
+ * use the setjmp buffer in this way I think that it
+ * is for the best then if changes occur compiles will
+ * break rather than just having new builds falling over
+ * mysteriously.
  */
 
-#define _JB_MAGIC       0
-#define _JB_SIGMASK     (_JB_MAGIC+1)
-#define _JB_FLOAT_BASE  (_JB_SIGMASK+1)
-#define _JB_FLOAT_STATE (_JB_FLOAT_BASE + (15-8+1)*2)
-#define _JB_CORE_BASE   (_JB_FLOAT_STATE+1)
-
 #define _JB_MAGIC__SETJMP	0x4278f500
 #define _JB_MAGIC_SETJMP	0x4278f501
+
+/* Valid for all jmp_buf's */
+
+#define _JB_MAGIC		 0
+#define _JB_REG_F4		 1
+#define _JB_REG_F5		 4
+#define _JB_REG_F6		 7
+#define _JB_REG_F7		10
+#define _JB_REG_FPSR		13
+#define _JB_REG_R4		14
+#define _JB_REG_R5		15
+#define _JB_REG_R6		16
+#define _JB_REG_R7		17
+#define _JB_REG_R8		18
+#define _JB_REG_R9		19
+#define _JB_REG_R10		20
+#define _JB_REG_R11		21
+#define _JB_REG_R12		22
+#define _JB_REG_R13		23
+#define _JB_REG_R14		24
+
+/* Only valid with the _JB_MAGIC_SETJMP magic */
+
+#define _JB_SIGMASK		25
diff -rupN sysroot-orig/usr/include/pthread.h sysroot/usr/include/pthread.h
--- sysroot-orig/usr/include/pthread.h	2012-10-22 13:52:50.000000000 +0800
+++ sysroot/usr/include/pthread.h	2012-08-23 18:50:57.000000000 +0800
@@ -306,4 +306,9 @@ extern void  __pthread_cleanup_pop(__pth
 } /* extern "C" */
 #endif
 
+/************ TO FIX ************/
+
+#define LONG_LONG_MAX __LONG_LONG_MAX__
+#define LONG_LONG_MIN (-__LONG_LONG_MAX__ - 1)
+
 #endif /* _PTHREAD_H_ */
diff -rupN sysroot-orig/usr/include/sys/stat.h sysroot/usr/include/sys/stat.h
--- sysroot-orig/usr/include/sys/stat.h	2012-10-04 20:52:36.000000000 +0800
+++ sysroot/usr/include/sys/stat.h	2012-09-10 14:44:44.000000000 +0800
@@ -55,8 +55,10 @@ struct stat {
     unsigned long long  st_rdev;
     unsigned char       __pad3[4];
 
+    unsigned char       __pad4[4];
     long long           st_size;
-    unsigned long	st_blksize;
+    unsigned long	    st_blksize;
+    unsigned char       __pad5[4];
     unsigned long long  st_blocks;
 
     unsigned long       st_atime;
