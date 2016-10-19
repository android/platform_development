/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_RSDISPATCH_H
#define ANDROID_RSDISPATCH_H

#include "rsDefines.h"

typedef const void* (*AllocationGetTypeFnPtr)(RsContext con, RsAllocation va);
typedef void (*TypeGetNativeDataFnPtr)(RsContext, RsType, uint32_t *typeData, uint32_t typeDataSize);
typedef void (*ElementGetNativeDataFnPtr)(RsContext, RsElement, uint32_t *elemData, uint32_t elemDataSize);
typedef void (*ElementGetSubElementsFnPtr)(RsContext, RsElement, uint32_t *ids, const char **names, uint32_t *arraySizes, uint32_t dataSize);
typedef RsDevice (*DeviceCreateFnPtr) ();
typedef void (*DeviceDestroyFnPtr) (RsDevice dev);
typedef void (*DeviceSetConfigFnPtr) (RsDevice dev, RsDeviceParam p, int32_t value);
typedef RsContext (*ContextCreateFnPtr)(RsDevice vdev, uint32_t version, uint32_t sdkVersion, RsContextType ct, uint32_t flags);
typedef void (*GetNameFnPtr)(RsContext, void * obj, const char **name);

typedef void (*ContextDestroyFnPtr) (RsContext);
typedef RsMessageToClientType (*ContextGetMessageFnPtr) (RsContext, void*, size_t, size_t*, size_t, uint32_t*, size_t);
typedef RsMessageToClientType (*ContextPeekMessageFnPtr) (RsContext, size_t*, size_t, uint32_t*, size_t);
typedef void (*ContextSendMessageFnPtr) (RsContext, uint32_t, const uint8_t*, size_t);
typedef void (*ContextInitToClientFnPtr) (RsContext);
typedef void (*ContextDeinitToClientFnPtr) (RsContext);
typedef RsType (*TypeCreateFnPtr) (RsContext, RsElement, uint32_t, uint32_t, uint32_t, bool, bool, uint32_t);
typedef RsAllocation (*AllocationCreateTypedFnPtr) (RsContext, RsType, RsAllocationMipmapControl, uint32_t, uintptr_t);
typedef RsAllocation (*AllocationCreateFromBitmapFnPtr) (RsContext, RsType, RsAllocationMipmapControl, const void*, size_t, uint32_t);
typedef RsAllocation (*AllocationCubeCreateFromBitmapFnPtr) (RsContext, RsType, RsAllocationMipmapControl, const void*, size_t, uint32_t);
typedef RsNativeWindow (*AllocationGetSurfaceFnPtr) (RsContext, RsAllocation);
typedef void (*AllocationSetSurfaceFnPtr) (RsContext, RsAllocation, RsNativeWindow);
typedef void (*ContextFinishFnPtr) (RsContext);
typedef void (*ContextDumpFnPtr) (RsContext, int32_t);
typedef void (*ContextSetPriorityFnPtr) (RsContext, int32_t);
typedef void (*AssignNameFnPtr) (RsContext, RsObjectBase, const char*, size_t);
typedef void (*ObjDestroyFnPtr) (RsContext, RsAsyncVoidPtr);
typedef RsElement (*ElementCreateFnPtr) (RsContext, RsDataType, RsDataKind, bool, uint32_t);
typedef RsElement (*ElementCreate2FnPtr) (RsContext, const RsElement*, size_t, const char**, size_t, const size_t*, const uint32_t*, size_t);
typedef void (*AllocationCopyToBitmapFnPtr) (RsContext, RsAllocation, void*, size_t);
typedef void (*Allocation1DDataFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, const void*, size_t);
typedef void (*Allocation1DElementDataFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, const void*, size_t, size_t);
typedef void (*Allocation2DDataFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, RsAllocationCubemapFace, uint32_t, uint32_t, const void*, size_t, size_t);
typedef void (*Allocation3DDataFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const void*, size_t, size_t);
typedef void (*AllocationGenerateMipmapsFnPtr) (RsContext, RsAllocation);
typedef void (*AllocationReadFnPtr) (RsContext, RsAllocation, void*, size_t);
typedef void (*Allocation1DReadFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, void*, size_t);
typedef void (*Allocation2DReadFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, RsAllocationCubemapFace, uint32_t, uint32_t, void*, size_t, size_t);
typedef void (*AllocationSyncAllFnPtr) (RsContext, RsAllocation, RsAllocationUsageType);
typedef void (*AllocationResize1DFnPtr) (RsContext, RsAllocation, uint32_t);
typedef void (*AllocationCopy2DRangeFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, RsAllocation, uint32_t, uint32_t, uint32_t, uint32_t);
typedef void (*AllocationCopy3DRangeFnPtr) (RsContext, RsAllocation, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, RsAllocation, uint32_t, uint32_t, uint32_t, uint32_t);
typedef RsSampler (*SamplerCreateFnPtr) (RsContext, RsSamplerValue, RsSamplerValue, RsSamplerValue, RsSamplerValue, RsSamplerValue, float);
typedef void (*ScriptBindAllocationFnPtr) (RsContext, RsScript, RsAllocation, uint32_t);
typedef void (*ScriptSetTimeZoneFnPtr) (RsContext, RsScript, const char*, size_t);
typedef void (*ScriptInvokeFnPtr) (RsContext, RsScript, uint32_t);
typedef void (*ScriptInvokeVFnPtr) (RsContext, RsScript, uint32_t, const void*, size_t);
typedef void (*ScriptForEachFnPtr) (RsContext, RsScript, uint32_t, RsAllocation, RsAllocation, const void*, size_t, const RsScriptCall*, size_t);
typedef void (*ScriptSetVarIFnPtr) (RsContext, RsScript, uint32_t, int);
typedef void (*ScriptSetVarObjFnPtr) (RsContext, RsScript, uint32_t, RsObjectBase);
typedef void (*ScriptSetVarJFnPtr) (RsContext, RsScript, uint32_t, int64_t);
typedef void (*ScriptSetVarFFnPtr) (RsContext, RsScript, uint32_t, float);
typedef void (*ScriptSetVarDFnPtr) (RsContext, RsScript, uint32_t, double);
typedef void (*ScriptSetVarVFnPtr) (RsContext, RsScript, uint32_t, const void*, size_t);
typedef void (*ScriptGetVarVFnPtr) (RsContext, RsScript, uint32_t, void*, size_t);
typedef void (*ScriptSetVarVEFnPtr) (RsContext, RsScript, uint32_t, const void*, size_t, RsElement, const size_t*, size_t);
typedef RsScript (*ScriptCCreateFnPtr) (RsContext, const char*, size_t, const char*, size_t, const char*, size_t);
typedef RsScript (*ScriptIntrinsicCreateFnPtr) (RsContext, uint32_t id, RsElement);
typedef RsScriptKernelID (*ScriptKernelIDCreateFnPtr) (RsContext, RsScript, int, int);
typedef RsScriptFieldID (*ScriptFieldIDCreateFnPtr) (RsContext, RsScript, int);
typedef RsScriptGroup (*ScriptGroupCreateFnPtr) (RsContext, RsScriptKernelID*, size_t, RsScriptKernelID*, size_t, RsScriptKernelID*, size_t, RsScriptFieldID*, size_t, const RsType*, size_t);
typedef void (*ScriptGroupSetOutputFnPtr) (RsContext, RsScriptGroup, RsScriptKernelID, RsAllocation);
typedef void (*ScriptGroupSetInputFnPtr) (RsContext, RsScriptGroup, RsScriptKernelID, RsAllocation);
typedef void (*ScriptGroupExecuteFnPtr) (RsContext, RsScriptGroup);
typedef void (*AllocationIoSendFnPtr) (RsContext, RsAllocation);
typedef void (*AllocationIoReceiveFnPtr) (RsContext, RsAllocation);

typedef struct {
    // inserted by hand from rs.h
    AllocationGetTypeFnPtr AllocationGetType;
    TypeGetNativeDataFnPtr TypeGetNativeData;
    ElementGetNativeDataFnPtr ElementGetNativeData;
    ElementGetSubElementsFnPtr ElementGetSubElements;

    DeviceCreateFnPtr DeviceCreate;
    DeviceDestroyFnPtr DeviceDestroy;
    DeviceSetConfigFnPtr DeviceSetConfig;
    ContextCreateFnPtr ContextCreate;
    GetNameFnPtr GetName;

    // generated from rs.spec
    ContextDestroyFnPtr ContextDestroy;
    ContextGetMessageFnPtr ContextGetMessage;
    ContextPeekMessageFnPtr ContextPeekMessage;
    ContextSendMessageFnPtr ContextSendMessage;
    ContextInitToClientFnPtr ContextInitToClient;
    ContextDeinitToClientFnPtr ContextDeinitToClient;
    TypeCreateFnPtr TypeCreate;
    AllocationCreateTypedFnPtr AllocationCreateTyped;
    AllocationCreateFromBitmapFnPtr AllocationCreateFromBitmap;
    AllocationCubeCreateFromBitmapFnPtr AllocationCubeCreateFromBitmap;
    AllocationGetSurfaceFnPtr AllocationGetSurface;
    AllocationSetSurfaceFnPtr AllocationSetSurface;
    ContextFinishFnPtr ContextFinish;
    ContextDumpFnPtr ContextDump;
    ContextSetPriorityFnPtr ContextSetPriority;
    AssignNameFnPtr AssignName;
    ObjDestroyFnPtr ObjDestroy;
    ElementCreateFnPtr ElementCreate;
    ElementCreate2FnPtr ElementCreate2;
    AllocationCopyToBitmapFnPtr AllocationCopyToBitmap;
    Allocation1DDataFnPtr Allocation1DData;
    Allocation1DElementDataFnPtr Allocation1DElementData;
    Allocation2DDataFnPtr Allocation2DData;
    Allocation3DDataFnPtr Allocation3DData;
    AllocationGenerateMipmapsFnPtr AllocationGenerateMipmaps;
    AllocationReadFnPtr AllocationRead;
    Allocation1DReadFnPtr Allocation1DRead;
    Allocation2DReadFnPtr Allocation2DRead;
    AllocationSyncAllFnPtr AllocationSyncAll;
    AllocationResize1DFnPtr AllocationResize1D;
    AllocationCopy2DRangeFnPtr AllocationCopy2DRange;
    AllocationCopy3DRangeFnPtr AllocationCopy3DRange;
    SamplerCreateFnPtr SamplerCreate;
    ScriptBindAllocationFnPtr ScriptBindAllocation;
    ScriptSetTimeZoneFnPtr ScriptSetTimeZone;
    ScriptInvokeFnPtr ScriptInvoke;
    ScriptInvokeVFnPtr ScriptInvokeV;
    ScriptForEachFnPtr ScriptForEach;
    ScriptSetVarIFnPtr ScriptSetVarI;
    ScriptSetVarObjFnPtr ScriptSetVarObj;
    ScriptSetVarJFnPtr ScriptSetVarJ;
    ScriptSetVarFFnPtr ScriptSetVarF;
    ScriptSetVarDFnPtr ScriptSetVarD;
    ScriptSetVarVFnPtr ScriptSetVarV;
    ScriptGetVarVFnPtr ScriptGetVarV;
    ScriptSetVarVEFnPtr ScriptSetVarVE;
    ScriptCCreateFnPtr ScriptCCreate;
    ScriptIntrinsicCreateFnPtr ScriptIntrinsicCreate;
    ScriptKernelIDCreateFnPtr ScriptKernelIDCreate;
    ScriptFieldIDCreateFnPtr ScriptFieldIDCreate;
    ScriptGroupCreateFnPtr ScriptGroupCreate;
    ScriptGroupSetOutputFnPtr ScriptGroupSetOutput;
    ScriptGroupSetInputFnPtr ScriptGroupSetInput;
    ScriptGroupExecuteFnPtr ScriptGroupExecute;
    AllocationIoSendFnPtr AllocationIoSend;
    AllocationIoReceiveFnPtr AllocationIoReceive;
} dispatchTable;

#endif
