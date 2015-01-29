/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BIONIC_FREEBSD_COMPAT_H_included
#define _BIONIC_FREEBSD_COMPAT_H_included

#define _BSD_SOURCE
#define REPLACE_GETOPT

/*
 * FreeBSD's libc has three symbols for every symbol:
 *
 *     __f will be the actual implementation.
 *     _f will be a weak reference to __f (used for calls to f from within the library).
 *     f will be a weak reference to __f (used for calls to f from outside the library).
 *
 * We collapse this into just the one symbol, f.
 */

/* Prevent weak reference generation. */
#define __weak_reference(sym,alias)

/* Ensure that the implementation itself gets the underscore-free name. */
#define __sleep sleep
#define __usleep usleep

/* Redirect internal C library calls to the public function. */
#define _nanosleep nanosleep

#endif
