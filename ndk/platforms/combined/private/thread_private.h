/* $OpenBSD: thread_private.h,v 1.18 2006/02/22 07:16:31 otto Exp $ */

/* PUBLIC DOMAIN: No Rights Reserved. Marco S Hyman <marc@snafu.org> */

#ifndef _THREAD_PRIVATE_H_
#define _THREAD_PRIVATE_H_

#include <pthread.h>

__BEGIN_DECLS

/*
 * This file defines the thread library interface to libc.  Thread
 * libraries must implement the functions described here for proper
 * inter-operation with libc.   libc contains weak versions of the
 * described functions for operation in a non-threaded environment.
 */

/*
 * helper macro to make unique names in the thread namespace
 */
#define __THREAD_NAME(name)	__CONCAT(_thread_tagname_,name)

struct __thread_private_tag_t {
    pthread_mutex_t    _private_lock;
    pthread_key_t      _private_key;
};

#define _THREAD_PRIVATE_MUTEX(name)  \
	static struct __thread_private_tag_t  __THREAD_NAME(name) = { PTHREAD_MUTEX_INITIALIZER, -1 }
#define _THREAD_PRIVATE_MUTEX_LOCK(name)  \
	pthread_mutex_lock( &__THREAD_NAME(name)._private_lock )
#define _THREAD_PRIVATE_MUTEX_UNLOCK(name) \
	pthread_mutex_unlock( &__THREAD_NAME(name)._private_lock )

/* Note that these aren't compatible with the usual OpenBSD ones which lazy-initialize! */
#define _MUTEX_LOCK(l) pthread_mutex_lock((pthread_mutex_t*) l)
#define _MUTEX_UNLOCK(l) pthread_mutex_unlock((pthread_mutex_t*) l)

__LIBC_HIDDEN__ void  _thread_atexit_lock(void);
__LIBC_HIDDEN__ void  _thread_atexit_unlock(void);

#define _ATEXIT_LOCK() _thread_atexit_lock()
#define _ATEXIT_UNLOCK() _thread_atexit_unlock()

__LIBC_HIDDEN__ void    _thread_arc4_lock(void);
__LIBC_HIDDEN__ void    _thread_arc4_unlock(void);

#define _ARC4_LOCK() _thread_arc4_lock()
#define _ARC4_UNLOCK() _thread_arc4_unlock()
#define _ARC4_ATFORK(f) pthread_atfork(NULL, NULL, (f))

__END_DECLS

#endif /* _THREAD_PRIVATE_H_ */
