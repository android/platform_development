/*
 * Copyright (C) 2009 The Android Open Source Project
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

/*
 * Contains declarations of types and constants used by malloc leak
 * detection code in both, libc and libc_malloc_debug libraries.
 */
#ifndef MALLOC_DEBUG_COMMON_H
#define MALLOC_DEBUG_COMMON_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "private/bionic_config.h"
#include "private/libc_logging.h"

#define HASHTABLE_SIZE      1543
#define BACKTRACE_SIZE      32
/* flag definitions, currently sharing storage with "size" */
#define SIZE_FLAG_ZYGOTE_CHILD  (1<<31)
#define SIZE_FLAG_MASK          (SIZE_FLAG_ZYGOTE_CHILD)

// This must match the alignment used by the malloc implementation.
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((size_t)(2 * sizeof(void *)))
#endif

// =============================================================================
// Structures
// =============================================================================

struct HashEntry {
    size_t slot;
    HashEntry* prev;
    HashEntry* next;
    size_t numEntries;
    // fields above "size" are NOT sent to the host
    size_t size;
    size_t allocations;
    uintptr_t backtrace[0];
};

struct HashTable {
    pthread_mutex_t lock;
    size_t count;
    HashEntry* slots[HASHTABLE_SIZE];
};

/* Entry in malloc dispatch table. */
typedef void* (*MallocDebugCalloc)(size_t, size_t);
typedef void (*MallocDebugFree)(void*);
typedef struct mallinfo (*MallocDebugMallinfo)();
typedef void* (*MallocDebugMalloc)(size_t);
typedef size_t (*MallocDebugMallocUsableSize)(const void*);
typedef void* (*MallocDebugMemalign)(size_t, size_t);
typedef int (*MallocDebugPosixMemalign)(void**, size_t, size_t);
#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
typedef void* (*MallocDebugPvalloc)(size_t);
#endif
typedef void* (*MallocDebugRealloc)(void*, size_t);
#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
typedef void* (*MallocDebugValloc)(size_t);
#endif

struct MallocDebug {
  MallocDebugCalloc calloc;
  MallocDebugFree free;
  MallocDebugMallinfo mallinfo;
  MallocDebugMalloc malloc;
  MallocDebugMallocUsableSize malloc_usable_size;
  MallocDebugMemalign memalign;
  MallocDebugPosixMemalign posix_memalign;
#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
  MallocDebugPvalloc pvalloc;
#endif
  MallocDebugRealloc realloc;
#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
  MallocDebugValloc valloc;
#endif
};

typedef bool (*MallocDebugInit)(HashTable*, const MallocDebug*);
typedef void (*MallocDebugFini)(int);

// =============================================================================
// log functions
// =============================================================================

#define debug_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_DEBUG, "malloc_leak_check", (format), ##__VA_ARGS__ )
#define error_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_ERROR, "malloc_leak_check", (format), ##__VA_ARGS__ )
#define info_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_INFO, "malloc_leak_check", (format), ##__VA_ARGS__ )

#endif  // MALLOC_DEBUG_COMMON_H
