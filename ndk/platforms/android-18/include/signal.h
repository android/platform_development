/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <asm/sigcontext.h>
#include <bits/pthread_types.h>
#include <bits/timespec.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#if defined(__LP64__) || defined(__mips__)
/* For 64-bit (and mips), the kernel's struct sigaction doesn't match the POSIX one,
 * so we need to expose our own and translate behind the scenes. */
#  define sigaction __kernel_sigaction
#  include <linux/signal.h>
#  undef sigaction
#else
/* For 32-bit, we're stuck with the definitions we already shipped,
 * even though they contain a sigset_t that's too small. */
#  include <linux/signal.h>
#endif

#include <sys/ucontext.h>
#define __BIONIC_HAVE_UCONTEXT_T

__BEGIN_DECLS

typedef int sig_atomic_t;

/* The arm and x86 kernel header files don't define _NSIG. */
#ifndef _KERNEL__NSIG
#define _KERNEL__NSIG 64
#endif

/* Userspace's NSIG is the kernel's _NSIG + 1. */
#define _NSIG (_KERNEL__NSIG + 1)
#define NSIG _NSIG

/* The kernel headers define SIG_DFL (0) and SIG_IGN (1) but not SIG_HOLD, since
 * SIG_HOLD is only used by the deprecated SysV signal API.
 */
#define SIG_HOLD ((sighandler_t)(uintptr_t)2)

/* We take a few real-time signals for ourselves. May as well use the same names as glibc. */
#define SIGRTMIN (__libc_current_sigrtmin())
#define SIGRTMAX (__libc_current_sigrtmax())
extern int __libc_current_sigrtmin(void) __INTRODUCED_IN(21);
extern int __libc_current_sigrtmax(void) __INTRODUCED_IN(21);

extern const char* const sys_siglist[];
extern const char* const sys_signame[]; /* BSD compatibility. */

typedef __sighandler_t sig_t; /* BSD compatibility. */
typedef __sighandler_t sighandler_t; /* glibc compatibility. */

#define si_timerid si_tid /* glibc compatibility. */

#if defined(__LP64__)

struct sigaction {
  unsigned int sa_flags;
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, struct siginfo*, void*);
  };
  sigset_t sa_mask;
  void (*sa_restorer)(void);
};

#elif defined(__mips__)

struct sigaction {
  unsigned int sa_flags;
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction) (int, struct siginfo*, void*);
  };
  sigset_t sa_mask;
};

#endif

extern int sigaction(int, const struct sigaction*, struct sigaction*);

__BIONIC_LEGACY_INLINE sighandler_t signal(int, sighandler_t);

extern int siginterrupt(int, int);

__BIONIC_LEGACY_INLINE int sigaddset(sigset_t*, int);
__BIONIC_LEGACY_INLINE int sigdelset(sigset_t*, int);
__BIONIC_LEGACY_INLINE int sigemptyset(sigset_t*);
__BIONIC_LEGACY_INLINE int sigfillset(sigset_t*);
__BIONIC_LEGACY_INLINE int sigismember(const sigset_t*, int);

extern int sigpending(sigset_t* _Nonnull);
extern int sigprocmask(int, const sigset_t*, sigset_t*);
extern int sigsuspend(const sigset_t* _Nonnull);
extern int sigwait(const sigset_t* _Nonnull, int* _Nonnull);

extern int sighold(int)
  __attribute__((deprecated("use sigprocmask() or pthread_sigmask() instead")))
  __INTRODUCED_IN_FUTURE;
extern int sigignore(int)
  __attribute__((deprecated("use sigaction() instead"))) __INTRODUCED_IN_FUTURE;
extern int sigpause(int)
  __attribute__((deprecated("use sigsuspend() instead"))) __INTRODUCED_IN_FUTURE;
extern int sigrelse(int)
  __attribute__((deprecated("use sigprocmask() or pthread_sigmask() instead")))
  __INTRODUCED_IN_FUTURE;
extern sighandler_t sigset(int, sighandler_t)
  __attribute__((deprecated("use sigaction() instead"))) __INTRODUCED_IN_FUTURE;

extern int raise(int);
extern int kill(pid_t, int);
extern int killpg(int, int);

extern int sigaltstack(const stack_t*, stack_t*);

extern void psiginfo(const siginfo_t*, const char*) __INTRODUCED_IN(17);
extern void psignal(int, const char*) __INTRODUCED_IN(17);

extern int pthread_kill(pthread_t, int);
extern int pthread_sigmask(int, const sigset_t*, sigset_t*);

extern int sigqueue(pid_t, int, const union sigval) __INTRODUCED_IN(23);
extern int sigtimedwait(const sigset_t*, siginfo_t*, const struct timespec*) __INTRODUCED_IN(23);
extern int sigwaitinfo(const sigset_t*, siginfo_t*) __INTRODUCED_IN(23);

__END_DECLS

#include <android/legacy_signal_inlines.h>

#endif /* _SIGNAL_H_ */
