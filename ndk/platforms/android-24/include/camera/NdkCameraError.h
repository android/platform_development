/*
 * Copyright (C) 2015 The Android Open Source Project
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

/**
 * @addtogroup Camera
 * @{
 */

/**
 * @file NdkCameraError.h
 */

/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_CAMERA_ERROR_H
#define _NDK_CAMERA_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACAMERA_OK = 0,

    ACAMERA_ERROR_BASE                  = -10000,
    ACAMERA_ERROR_UNKNOWN               = ACAMERA_ERROR_BASE,
    ACAMERA_ERROR_UNSUPPORTED           = ACAMERA_ERROR_BASE - 1,
    ACAMERA_ERROR_INVALID_PARAMETER     = ACAMERA_ERROR_BASE - 2,
    ACAMERA_ERROR_CAMERA_DISCONNECTED   = ACAMERA_ERROR_BASE - 3,
    ACAMERA_ERROR_NOT_ENOUGH_MEMORY     = ACAMERA_ERROR_BASE - 4,
    ACAMERA_ERROR_METADATA_NOT_FOUND    = ACAMERA_ERROR_BASE - 5,
    ACAMERA_ERROR_CAMERA_DEVICE         = ACAMERA_ERROR_BASE - 6,
    ACAMERA_ERROR_CAMERA_SERVICE        = ACAMERA_ERROR_BASE - 7,
    ACAMERA_ERROR_CAMERA_REQUEST        = ACAMERA_ERROR_BASE - 8,
    ACAMERA_ERROR_CAMERA_RESULT         = ACAMERA_ERROR_BASE - 9,
    ACAMERA_ERROR_CAMERA_BUFFER         = ACAMERA_ERROR_BASE - 10,
    ACAMERA_ERROR_SESSION_CLOSED        = ACAMERA_ERROR_BASE - 11,
    ACAMERA_ERROR_SESSION_NOT_DRAINED   = ACAMERA_ERROR_BASE - 12,
    ACAMERA_ERROR_INVALID_OPERATION     = ACAMERA_ERROR_BASE - 13,
    ACAMERA_ERROR_TIMEOUT               = ACAMERA_ERROR_BASE - 14,
    ACAMERA_ERROR_STREAM_CONFIGURE_FAIL = ACAMERA_ERROR_BASE - 15,
    ACAMERA_ERROR_CAMERA_IN_USE         = ACAMERA_ERROR_BASE - 16,
    ACAMERA_ERROR_MAX_CAMERA_IN_USE     = ACAMERA_ERROR_BASE - 17,
    ACAMERA_ERROR_CAMERA_DISABLED       = ACAMERA_ERROR_BASE - 18,
    ACAMERA_ERROR_PERMISSION_DENIED     = ACAMERA_ERROR_BASE - 19,
} camera_status_t;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NDK_CAMERA_ERROR_H

/** @} */
