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

#ifndef ANDROID_RSCPPSTRUCTS_H
#define ANDROID_RSCPPSTRUCTS_H

#include "rsDefines.h"
#include "util/RefBase.h"
#include "rsDispatch.h"

#include <vector>
#include <string>

/**
 * Every row in an RS allocation is guaranteed to be aligned by this amount, and
 * every row in a user-backed allocation must be aligned by this amount.
 */
#define RS_CPU_ALLOCATION_ALIGNMENT 16

namespace android {
namespace RSC {

typedef void (*ErrorHandlerFunc_t)(uint32_t errorNum, const char *errorText);
typedef void (*MessageHandlerFunc_t)(uint32_t msgNum, const void *msgData, size_t msgLen);

class RS;
class BaseObj;
class Element;
class Type;
class Allocation;
class Script;
class ScriptC;
class Sampler;

/**
 * Possible error codes used by RenderScript. Once a status other than RS_SUCCESS
 * is returned, the RenderScript context is considered dead and cannot perform any
 * additional work.
 */
 enum RSError {
     RS_SUCCESS = 0,                 ///< No error
     RS_ERROR_INVALID_PARAMETER = 1, ///< An invalid parameter was passed to a function
     RS_ERROR_RUNTIME_ERROR = 2,     ///< The RenderScript driver returned an error; this is
                                     ///< often indicative of a kernel that crashed
     RS_ERROR_INVALID_ELEMENT = 3,   ///< An invalid Element was passed to a function
     RS_ERROR_MAX = 9999

 };

 /**
  * YUV formats supported by the RenderScript API.
  */
 enum RSYuvFormat {
     RS_YUV_NONE = 0, ///< No YUV data
     RS_YUV_YV12 = 1, ///< YUV data in YV12 format
     RS_YUV_NV21 = 2, ///< YUV data in NV21 format
     RS_YUV_MAX = 3
 };

 /**
  * Flags that can control RenderScript behavior on a per-context level.
  */
 enum RSInitFlags {
     RS_INIT_SYNCHRONOUS = 1, ///< All RenderScript calls will be synchronous. May reduce latency.
     RS_INIT_LOW_LATENCY = 2, ///< Prefer low latency devices over potentially higher throughput devices.
     RS_INIT_MAX = 4
 };

 /**
  * The RenderScript context. This class controls initialization, resource management, and teardown.
  */
 class RS : public android::RSC::LightRefBase<RS> {

 public:
    RS();
    virtual ~RS();

    /**
     * Initializes a RenderScript context. A context must be initialized before it can be used.
     * @param[in] name Directory name to be used by this context. This should be equivalent to
     * Context.getCacheDir().
     * @param[in] flags Optional flags for this context.
     * @return true on success
     */
    bool init(std::string name, uint32_t flags = 0);

    /**
     * Sets the error handler function for this context. This error handler is
     * called whenever an error is set.
     *
     * @param[in] func Error handler function
     */
    void setErrorHandler(ErrorHandlerFunc_t func);

    /**
     * Returns the current error handler function for this context.
     *
     * @return pointer to current error handler function or NULL if not set
     */
    ErrorHandlerFunc_t getErrorHandler() { return mErrorFunc; }

    /**
     * Sets the message handler function for this context. This message handler
     * is called whenever a message is sent from a RenderScript kernel.
     *
     *  @param[in] func Message handler function
     */
    void setMessageHandler(MessageHandlerFunc_t func);

    /**
     * Returns the current message handler function for this context.
     *
     * @return pointer to current message handler function or NULL if not set
     */
    MessageHandlerFunc_t getMessageHandler() { return mMessageFunc; }

    /**
     * Returns current status for the context.
     *
     * @return current error
     */
    RSError getError();

    /**
     * Waits for any currently running asynchronous operations to finish. This
     * should only be used for performance testing and timing.
     */
    void finish();

    RsContext getContext() { return mContext; }
    void throwError(RSError error, const char *errMsg);

    static dispatchTable* dispatch;

 private:
    static bool usingNative;
    static bool initDispatch(int targetApi);

    bool init(std::string &name, int targetApi, uint32_t flags);
    static void * threadProc(void *);

    static bool gInitialized;
    static pthread_mutex_t gInitMutex;

    pthread_t mMessageThreadId;
    pid_t mNativeMessageThreadId;
    bool mMessageRun;

    RsDevice mDev;
    RsContext mContext;
    RSError mCurrentError;

    ErrorHandlerFunc_t mErrorFunc;
    MessageHandlerFunc_t mMessageFunc;
    bool mInit;

    std::string mCacheDir;

    struct {
        sp<const Element> U8;
        sp<const Element> U8_2;
        sp<const Element> U8_3;
        sp<const Element> U8_4;
        sp<const Element> I8;
        sp<const Element> I8_2;
        sp<const Element> I8_3;
        sp<const Element> I8_4;
        sp<const Element> U16;
        sp<const Element> U16_2;
        sp<const Element> U16_3;
        sp<const Element> U16_4;
        sp<const Element> I16;
        sp<const Element> I16_2;
        sp<const Element> I16_3;
        sp<const Element> I16_4;
        sp<const Element> U32;
        sp<const Element> U32_2;
        sp<const Element> U32_3;
        sp<const Element> U32_4;
        sp<const Element> I32;
        sp<const Element> I32_2;
        sp<const Element> I32_3;
        sp<const Element> I32_4;
        sp<const Element> U64;
        sp<const Element> U64_2;
        sp<const Element> U64_3;
        sp<const Element> U64_4;
        sp<const Element> I64;
        sp<const Element> I64_2;
        sp<const Element> I64_3;
        sp<const Element> I64_4;
        sp<const Element> F32;
        sp<const Element> F32_2;
        sp<const Element> F32_3;
        sp<const Element> F32_4;
        sp<const Element> F64;
        sp<const Element> F64_2;
        sp<const Element> F64_3;
        sp<const Element> F64_4;
        sp<const Element> BOOLEAN;

        sp<const Element> ELEMENT;
        sp<const Element> TYPE;
        sp<const Element> ALLOCATION;
        sp<const Element> SAMPLER;
        sp<const Element> SCRIPT;
        sp<const Element> MESH;
        sp<const Element> PROGRAM_FRAGMENT;
        sp<const Element> PROGRAM_VERTEX;
        sp<const Element> PROGRAM_RASTER;
        sp<const Element> PROGRAM_STORE;

        sp<const Element> A_8;
        sp<const Element> RGB_565;
        sp<const Element> RGB_888;
        sp<const Element> RGBA_5551;
        sp<const Element> RGBA_4444;
        sp<const Element> RGBA_8888;

        sp<const Element> YUV;

        sp<const Element> MATRIX_4X4;
        sp<const Element> MATRIX_3X3;
        sp<const Element> MATRIX_2X2;
    } mElements;

    struct {
        sp<const Sampler> CLAMP_NEAREST;
        sp<const Sampler> CLAMP_LINEAR;
        sp<const Sampler> CLAMP_LINEAR_MIP_LINEAR;
        sp<const Sampler> WRAP_NEAREST;
        sp<const Sampler> WRAP_LINEAR;
        sp<const Sampler> WRAP_LINEAR_MIP_LINEAR;
        sp<const Sampler> MIRRORED_REPEAT_NEAREST;
        sp<const Sampler> MIRRORED_REPEAT_LINEAR;
        sp<const Sampler> MIRRORED_REPEAT_LINEAR_MIP_LINEAR;
    } mSamplers;
    friend class Sampler;
    friend class Element;
    friend class ScriptC;
};

 /**
  * Base class for all RenderScript objects. Not for direct use by developers.
  */
class BaseObj : public android::RSC::LightRefBase<BaseObj> {
public:
    void * getID() const;
    virtual ~BaseObj();
    virtual void updateFromNative();
    virtual bool equals(sp<const BaseObj> obj);

protected:
    void *mID;
    RS* mRS;
    std::string mName;

    BaseObj(void *id, sp<RS> rs);
    void checkValid();

    static void * getObjID(sp<const BaseObj> o);

};

 /**
  * This class provides the primary method through which data is passed to and
  * from RenderScript kernels. An Allocation provides the backing store for a
  * given Type.
  *
  * An Allocation also contains a set of usage flags that denote how the
  * Allocation could be used. For example, an Allocation may have usage flags
  * specifying that it can be used from a script as well as input to a
  * Sampler. A developer must synchronize across these different usages using
  * syncAll(int) in order to ensure that different users of the Allocation have
  * a consistent view of memory. For example, in the case where an Allocation is
  * used as the output of one kernel and as Sampler input in a later kernel, a
  * developer must call syncAll(RS_ALLOCATION_USAGE_SCRIPT) prior to launching the
  * second kernel to ensure correctness.
  */
class Allocation : public BaseObj {
protected:
    sp<const Type> mType;
    uint32_t mUsage;
    sp<Allocation> mAdaptedAllocation;

    bool mConstrainedLOD;
    bool mConstrainedFace;
    bool mConstrainedY;
    bool mConstrainedZ;
    bool mReadAllowed;
    bool mWriteAllowed;
    uint32_t mSelectedY;
    uint32_t mSelectedZ;
    uint32_t mSelectedLOD;
    RsAllocationCubemapFace mSelectedFace;

    uint32_t mCurrentDimX;
    uint32_t mCurrentDimY;
    uint32_t mCurrentDimZ;
    uint32_t mCurrentCount;

    void * getIDSafe() const;
    void updateCacheInfo(sp<const Type> t);

    Allocation(void *id, sp<RS> rs, sp<const Type> t, uint32_t usage);

    void validateIsInt32();
    void validateIsInt16();
    void validateIsInt8();
    void validateIsFloat32();
    void validateIsObject();

    virtual void updateFromNative();

    void validate2DRange(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h);
    void validate3DRange(uint32_t xoff, uint32_t yoff, uint32_t zoff,
                         uint32_t w, uint32_t h, uint32_t d);

public:

    /**
     * Return Type for the allocation.
     * @return pointer to underlying Type
     */
    sp<const Type> getType() const {
        return mType;
    }

    /**
     * Propagate changes from one usage of the Allocation to other usages of the Allocation.
     * @param[in] srcLocation source location with changes to propagate elsewhere
     */
    void syncAll(RsAllocationUsageType srcLocation);
    void ioSendOutput();
    void ioGetInput();

    /**
     * Generate a mipmap chain. This is only valid if the Type of the Allocation
     * includes mipmaps. This function will generate a complete set of mipmaps
     * from the top level LOD and place them into the script memory space. If
     * the Allocation is also using other memory spaces, a call to
     * syncAll(Allocation.USAGE_SCRIPT) is required.
     */
    void generateMipmaps();

    /**
     * Copy an array into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data array from which to copy
     */
    void copy1DRangeFrom(uint32_t off, size_t count, const void *data);

    /**
     * Copy part of an Allocation into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data Allocation from which to copy
     * @param[in] dataOff offset of first Element in data to copy
     */
    void copy1DRangeFrom(uint32_t off, size_t count, sp<const Allocation> data, uint32_t dataOff);

    /**
     * Copy an array into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data array from which to copy
     */
    void copy1DRangeTo(uint32_t off, size_t count, void *data);

    /**
     * Copy entire array to an Allocation.
     * @param[in] data array from which to copy
     */
    void copy1DFrom(const void* data);

    /**
     * Copy entire Allocation to an array.
     * @param[in] data destination array
     */
    void copy1DTo(void* data);

    /**
     * Copy from an array into a rectangular region in this Allocation. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data Array from which to copy
     */
    void copy2DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         const void *data);

    /**
     * Copy from this Allocation into a rectangular region in an array. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to copy from this Allocation
     * @param[in] yoff Y offset of region to copy from this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data destination array
     */
    void copy2DRangeTo(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                       void *data);

    /**
     * Copy from an Allocation into a rectangular region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data Allocation from which to copy
     * @param[in] dataXoff X offset of region to copy from in data
     * @param[in] dataYoff Y offset of region to copy from in data
     */
    void copy2DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         sp<const Allocation> data, uint32_t dataXoff, uint32_t dataYoff);

    /**
     * Copy from a strided array into a rectangular region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data array from which to copy
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                           const void *data, size_t stride);

    /**
     * Copy from a strided array into this Allocation.
     * @param[in] data array from which to copy
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedFrom(const void *data, size_t stride);

    /**
     * Copy from a rectangular region in this Allocation into a strided array.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data destination array
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedTo(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         void *data, size_t stride);

    /**
     * Copy this Allocation into a strided array.
     * @param[in] data destination array
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedTo(void *data, size_t stride);


    /**
     * Copy from an array into a 3D region in this Allocation. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] zoff Z offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] d Depth of region to update
     * @param[in] data Array from which to copy
     */
    void copy3DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t zoff, uint32_t w,
                         uint32_t h, uint32_t d, const void* data);

    /**
     * Copy from an Allocation into a 3D region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] zoff Z offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] d Depth of region to update
     * @param[in] data Allocation from which to copy
     * @param[in] dataXoff X offset of region in data to copy from
     * @param[in] dataYoff Y offset of region in data to copy from
     * @param[in] dataZoff Z offset of region in data to copy from
     */
    void copy3DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t zoff,
                         uint32_t w, uint32_t h, uint32_t d,
                         sp<const Allocation> data,
                         uint32_t dataXoff, uint32_t dataYoff, uint32_t dataZoff);

    /**
     * Creates an Allocation for use by scripts with a given Type.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] mipmaps desired mipmap behavior for the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createTyped(sp<RS> rs, sp<const Type> type,
                                   RsAllocationMipmapControl mipmaps, uint32_t usage);

    /**
     * Creates an Allocation for use by scripts with a given Type and a backing pointer. For use
     * with RS_ALLOCATION_USAGE_SHARED.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] mipmaps desired mipmap behavior for the Allocation
     * @param[in] usage usage for the Allocation
     * @param[in] pointer existing backing store to use for this Allocation if possible
     * @return new Allocation
     */
    static sp<Allocation> createTyped(sp<RS> rs, sp<const Type> type,
                                   RsAllocationMipmapControl mipmaps, uint32_t usage, void * pointer);

    /**
     * Creates an Allocation for use by scripts with a given Type with no mipmaps.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createTyped(sp<RS> rs, sp<const Type> type,
                                   uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);
    /**
     * Creates an Allocation with a specified number of given elements.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] e Element used in the Allocation
     * @param[in] count Number of elements of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createSized(sp<RS> rs, sp<const Element> e, size_t count,
                                   uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);

    /**
     * Creates a 2D Allocation with a specified number of given elements.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] e Element used in the Allocation
     * @param[in] x Width in Elements of the Allocation
     * @param[in] y Height of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createSized2D(sp<RS> rs, sp<const Element> e,
                                        size_t x, size_t y,
                                        uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);


};

 /**
  * An Element represents one item within an Allocation. An Element is roughly
  * equivalent to a C type in a RenderScript kernel. Elements may be basic
  * or complex. Some basic elements are:

  * - A single float value (equivalent to a float in a kernel)
  * - A four-element float vector (equivalent to a float4 in a kernel)
  * - An unsigned 32-bit integer (equivalent to an unsigned int in a kernel)
  * - A single signed 8-bit integer (equivalent to a char in a kernel)

  * Basic Elements are comprised of a Element.DataType and a
  * Element.DataKind. The DataType encodes C type information of an Element,
  * while the DataKind encodes how that Element should be interpreted by a
  * Sampler. Note that Allocation objects with DataKind USER cannot be used as
  * input for a Sampler. In general, Allocation objects that are intended for
  * use with a Sampler should use bitmap-derived Elements such as
  * Element::RGBA_8888.
 */


class Element : public BaseObj {
public:
    bool isComplex();

    /**
     * Elements could be simple, such as an int or a float, or a structure with
     * multiple sub-elements, such as a collection of floats, float2,
     * float4. This function returns zero for simple elements or the number of
     * sub-elements otherwise.
     * @return number of sub-elements
     */
    size_t getSubElementCount() {
        return mVisibleElementMap.size();
    }

    /**
     * For complex Elements, this returns the sub-element at a given index.
     * @param[in] index index of sub-element
     * @return sub-element
     */
    sp<const Element> getSubElement(uint32_t index);

    /**
     * For complex Elements, this returns the name of the sub-element at a given
     * index.
     * @param[in] index index of sub-element
     * @return name of sub-element
     */
    const char * getSubElementName(uint32_t index);

    /**
     * For complex Elements, this returns the size of the sub-element at a given
     * index.
     * @param[in] index index of sub-element
     * @return size of sub-element
     */
    size_t getSubElementArraySize(uint32_t index);

    /**
     * Returns the location of a sub-element within a complex Element.
     * @param[in] index index of sub-element
     * @return offset in bytes
     */
    uint32_t getSubElementOffsetBytes(uint32_t index);

    /**
     * Returns the data type used for the Element.
     * @return data type
     */
    RsDataType getDataType() const {
        return mType;
    }

    /**
     * Returns the data kind used for the Element.
     * @return data kind
     */
    RsDataKind getDataKind() const {
        return mKind;
    }

    /**
     * Returns the size in bytes of the Element.
     * @return size in bytes
     */
    size_t getSizeBytes() const {
        return mSizeBytes;
    }

    /**
     * Returns the number of vector components for this Element.
     * @return number of vector components
     */
    uint32_t getVectorSize() const {
        return mVectorSize;
    }

    /**
     * Utility function for returning an Element containing a single bool.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> BOOLEAN(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single unsigned char.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single signed char.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single unsigned short.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single signed short.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single unsigned int.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single signed int.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single unsigned long long.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single signed long long.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single float.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single double.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single Element.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> ELEMENT(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single Type.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> TYPE(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single Allocation.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> ALLOCATION(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single Sampler.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> SAMPLER(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a single Script.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> SCRIPT(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an ALPHA_8 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> A_8(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an RGB_565 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGB_565(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an RGB_888 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGB_888(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an RGBA_5551 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_5551(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an RGBA_4444 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_4444(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an RGBA_8888 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_8888(sp<RS> rs);

    /**
     * Utility function for returning an Element containing a float2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a float3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a float4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a double2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a double3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a double4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uchar2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uchar3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uchar4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a char2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a char3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a char4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ushort2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ushort3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ushort4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a short2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a short3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a short4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uint2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uint3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a uint4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an int2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an int3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an int4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ulong2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ulong3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a ulong4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a long2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_2(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a long3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a long4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing a YUV pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> YUV(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_4x4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_4X4(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_3x3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_3X3(sp<RS> rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_2x2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_2X2(sp<RS> rs);

    void updateFromNative();

    /**
     * Create an Element with a given DataType.
     * @param[in] rs RenderScript context
     * @param[in] dt data type
     * @return Element
     */
    static sp<const Element> createUser(sp<RS> rs, RsDataType dt);
    /**
     * Create a vector Element with the given DataType
     * @param[in] rs RenderScript
     * @param[in] dt DataType
     * @param[in] size vector size
     * @return Element
     */
    static sp<const Element> createVector(sp<RS> rs, RsDataType dt, uint32_t size);
    /**
     * Create an Element with a given DataType and DataKind.
     * @param[in] rs RenderScript context
     * @param[in] dt DataType
     * @param[in] dk DataKind
     * @return Element
     */
    static sp<const Element> createPixel(sp<RS> rs, RsDataType dt, RsDataKind dk);

    /**
     * Returns true if the Element can interoperate with this Element.
     * @param[in] e Element to compare
     * @return true if Elements can interoperate
     */
    bool isCompatible(sp<const Element>e) const;

    /**
     * Builder class for producing complex elements with matching field and name
     * pairs. The builder starts empty. The order in which elements are added is
     * retained for the layout in memory.
     */
    class Builder {
    private:
        RS* mRS;
        std::vector<sp<Element> > mElements;
        std::vector<std::string> mElementNames;
        std::vector<uint32_t> mArraySizes;
        bool mSkipPadding;

    public:
        Builder(sp<RS> rs);
        ~Builder();
        void add(sp<Element> e, std::string &name, uint32_t arraySize = 1);
        sp<const Element> create();
    };

protected:
    Element(void *id, sp<RS> rs,
            std::vector<sp<Element> > &elements,
            std::vector<std::string> &elementNames,
            std::vector<uint32_t> &arraySizes);
    Element(void *id, sp<RS> rs, RsDataType dt, RsDataKind dk, bool norm, uint32_t size);
    Element(sp<RS> rs);
    virtual ~Element();

private:
    void updateVisibleSubElements();

    std::vector<sp<Element> > mElements;
    std::vector<std::string> mElementNames;
    std::vector<uint32_t> mArraySizes;
    std::vector<uint32_t> mVisibleElementMap;
    std::vector<uint32_t> mOffsetInBytes;

    RsDataType mType;
    RsDataKind mKind;
    bool mNormalized;
    size_t mSizeBytes;
    size_t mVectorSize;
};

class FieldPacker {
protected:
    unsigned char* mData;
    size_t mPos;
    size_t mLen;

public:
    FieldPacker(size_t len)
        : mPos(0), mLen(len) {
            mData = new unsigned char[len];
        }

    virtual ~FieldPacker() {
        delete [] mData;
    }

    void align(size_t v) {
        if ((v & (v - 1)) != 0) {
            //            ALOGE("Non-power-of-two alignment: %zu", v);
            return;
        }

        while ((mPos & (v - 1)) != 0) {
            mData[mPos++] = 0;
        }
    }

    void reset() {
        mPos = 0;
    }

    void reset(size_t i) {
        if (i >= mLen) {
            //            ALOGE("Out of bounds: i (%zu) >= len (%zu)", i, mLen);
            return;
        }
        mPos = i;
    }

    void skip(size_t i) {
        size_t res = mPos + i;
        if (res > mLen) {
            //            ALOGE("Exceeded buffer length: i (%zu) > len (%zu)", i, mLen);
            return;
        }
        mPos = res;
    }

    void* getData() const {
        return mData;
    }

    size_t getLength() const {
        return mLen;
    }

    template <typename T>
        void add(T t) {
        align(sizeof(t));
        if (mPos + sizeof(t) <= mLen) {
            memcpy(&mData[mPos], &t, sizeof(t));
            mPos += sizeof(t);
        }
    }

    /*
      void add(rs_matrix4x4 m) {
      for (size_t i = 0; i < 16; i++) {
      add(m.m[i]);
      }
      }

      void add(rs_matrix3x3 m) {
      for (size_t i = 0; i < 9; i++) {
      add(m.m[i]);
      }
      }

      void add(rs_matrix2x2 m) {
      for (size_t i = 0; i < 4; i++) {
      add(m.m[i]);
      }
      }
    */

    void add(sp<BaseObj> obj) {
        if (obj != NULL) {
            add((uint32_t) (uintptr_t) obj->getID());
        } else {
            add((uint32_t) 0);
        }
    }
};

/**
 * A Type describes the Element and dimensions used for an Allocation or a
 * parallel operation.
 *
 * A Type always includes an Element and an X dimension. A Type may be
 * multidimensional, up to three dimensions. A nonzero value in the Y or Z
 * dimensions indicates that the dimension is present. Note that a Type with
 * only a given X dimension and a Type with the same X dimension but Y = 1 are
 * not equivalent.
 *
 * A Type also supports inclusion of level of detail (LOD) or cube map
 * faces. LOD and cube map faces are booleans to indicate present or not
 * present.
 *
 * A Type also supports YUV format information to support an Allocation in a YUV
 * format. The YUV formats supported are YV12 and NV21.
 */
class Type : public BaseObj {
protected:
    friend class Allocation;

    uint32_t mDimX;
    uint32_t mDimY;
    uint32_t mDimZ;
    RSYuvFormat mYuvFormat;
    bool mDimMipmaps;
    bool mDimFaces;
    size_t mElementCount;
    sp<const Element> mElement;

    Type(void *id, sp<RS> rs);

    void calcElementCount();
    virtual void updateFromNative();

public:

    /**
     * Returns the YUV format.
     * @return YUV format of the Allocation
     */
    RSYuvFormat getYuvFormat() const {
        return mYuvFormat;
    }

    /**
     * Returns the Element of the Allocation.
     * @return YUV format of the Allocation
     */
    sp<const Element> getElement() const {
        return mElement;
    }

    /**
     * Returns the X dimension of the Allocation.
     * @return X dimension of the allocation
     */
    uint32_t getX() const {
        return mDimX;
    }

    /**
     * Returns the Y dimension of the Allocation.
     * @return Y dimension of the allocation
     */
    uint32_t getY() const {
        return mDimY;
    }

    /**
     * Returns the Z dimension of the Allocation.
     * @return Z dimension of the allocation
     */
    uint32_t getZ() const {
        return mDimZ;
    }

    /**
     * Returns true if the Allocation has mipmaps.
     * @return true if the Allocation has mipmaps
     */
    bool hasMipmaps() const {
        return mDimMipmaps;
    }

    /**
     * Returns true if the Allocation is a cube map
     * @return true if the Allocation is a cube map
     */
    bool hasFaces() const {
        return mDimFaces;
    }

    /**
     * Returns number of accessible Elements in the Allocation
     * @return number of accessible Elements in the Allocation
     */
    size_t getCount() const {
        return mElementCount;
    }

    /**
     * Returns size in bytes of all Elements in the Allocation
     * @return size in bytes of all Elements in the Allocation
     */
    size_t getSizeBytes() const {
        return mElementCount * mElement->getSizeBytes();
    }

    /**
     * Creates a new Type with the given Element and dimensions.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @param[in] dimX X dimension
     * @param[in] dimY Y dimension
     * @param[in] dimZ Z dimension
     * @return new Type
     */
    static sp<const Type> create(sp<RS> rs, sp<const Element> e, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

    class Builder {
    protected:
        RS* mRS;
        uint32_t mDimX;
        uint32_t mDimY;
        uint32_t mDimZ;
        RSYuvFormat mYuvFormat;
        bool mDimMipmaps;
        bool mDimFaces;
        sp<const Element> mElement;

    public:
        Builder(sp<RS> rs, sp<const Element> e);

        void setX(uint32_t value);
        void setY(uint32_t value);
        void setZ(uint32_t value);
        void setYuvFormat(RSYuvFormat format);
        void setMipmaps(bool value);
        void setFaces(bool value);
        sp<const Type> create();
    };

};

/**
 * The parent class for all executable Scripts. This should not be used by applications.
 */
class Script : public BaseObj {
private:

protected:
    Script(void *id, sp<RS> rs);
    void forEach(uint32_t slot, sp<const Allocation> in, sp<const Allocation> out,
            const void *v, size_t) const;
    void bindAllocation(sp<Allocation> va, uint32_t slot) const;
    void setVar(uint32_t index, const void *, size_t len) const;
    void setVar(uint32_t index, sp<const BaseObj> o) const;
    void invoke(uint32_t slot, const void *v, size_t len) const;


    void invoke(uint32_t slot) const {
        invoke(slot, NULL, 0);
    }
    void setVar(uint32_t index, float v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, double v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, int32_t v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, int64_t v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, bool v) const {
        setVar(index, &v, sizeof(v));
    }

public:
    class FieldBase {
    protected:
        sp<const Element> mElement;
        sp<Allocation> mAllocation;

        void init(sp<RS> rs, uint32_t dimx, uint32_t usages = 0);

    public:
        sp<const Element> getElement() {
            return mElement;
        }

        sp<const Type> getType() {
            return mAllocation->getType();
        }

        sp<const Allocation> getAllocation() {
            return mAllocation;
        }

        //void updateAllocation();
    };
};

/**
 * The parent class for all user-defined scripts. This is intended to be used by auto-generated code only.
 */
class ScriptC : public Script {
protected:
    ScriptC(sp<RS> rs,
            const void *codeTxt, size_t codeLength,
            const char *cachedName, size_t cachedNameLength,
            const char *cacheDir, size_t cacheDirLength);

};

/**
 * The parent class for all script intrinsics. Intrinsics provide highly optimized implementations of
 * basic functions. This is not intended to be used directly.
 */
class ScriptIntrinsic : public Script {
 protected:
    sp<const Element> mElement;
    ScriptIntrinsic(sp<RS> rs, int id, sp<const Element> e);
    virtual ~ScriptIntrinsic();
};

/**
 * Intrinsic for converting RGB to RGBA by using a 3D lookup table. The incoming
 * r,g,b values are use as normalized x,y,z coordinates into a 3D
 * allocation. The 8 nearest values are sampled and linearly interpolated. The
 * result is placed in the output.
 */
class ScriptIntrinsic3DLUT : public ScriptIntrinsic {
 private:
    ScriptIntrinsic3DLUT(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8_4. Default lookup table is identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsic
     */
    static sp<ScriptIntrinsic3DLUT> create(sp<RS> rs, sp<const Element> e);

    /**
     * Launch the intrinsic.
     * @param[in] ain input Allocation
     * @param[in] aout output Allocation
     */
    void forEach(sp<Allocation> ain, sp<Allocation> aout);

    /**
     * Sets the lookup table. The lookup table must use the same Element as the
     * intrinsic.
     * @param[in] lut new lookup table
     */
    void setLUT(sp<Allocation> lut);
};

/**
 * Intrinsic kernel for blending two Allocations.
 */
class ScriptIntrinsicBlend : public ScriptIntrinsic {
 private:
    ScriptIntrinsicBlend(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8_4.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicBlend
     */
    static sp<ScriptIntrinsicBlend> create(sp<RS> rs, sp<const Element> e);
    /**
     * sets dst = {0, 0, 0, 0}
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachClear(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = src
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrc(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = dst (NOP)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDst(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = src + dst * (1.0 - src.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcOver(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = dst + src * (1.0 - dst.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstOver(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = src * dst.a
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcIn(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = dst * src.a
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstIn(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = src * (1.0 - dst.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcOut(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = dst * (1.0 - src.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstOut(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst.rgb = src.rgb * dst.a + (1.0 - src.a) * dst.rgb
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcAtop(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst.rgb = dst.rgb * src.a + (1.0 - dst.a) * src.rgb
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstAtop(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = {src.r ^ dst.r, src.g ^ dst.g, src.b ^ dst.b, src.a ^ dst.a}
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachXor(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = src * dst
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachMultiply(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = min(src + dst, 1.0)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachAdd(sp<Allocation> in, sp<Allocation> out);
    /**
     * Sets dst = max(dst - src, 0.0)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSubtract(sp<Allocation> in, sp<Allocation> out);
};

/**
 * Intrinsic Gausian blur filter. Applies a Gaussian blur of the specified
 * radius to all elements of an Allocation.
 */
class ScriptIntrinsicBlur : public ScriptIntrinsic {
 private:
    ScriptIntrinsicBlur(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8 and U8_4.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicBlur
     */
    static sp<ScriptIntrinsicBlur> create(sp<RS> rs, sp<const Element> e);
    /**
     * Sets the input of the blur.
     * @param[in] in input Allocation
     */
    void setInput(sp<Allocation> in);
    /**
     * Runs the intrinsic.
     * @param[in] output Allocation
     */
    void forEach(sp<Allocation> out);
    /**
     * Sets the radius of the blur. The supported range is 0 < radius <= 25.
     * @param[in] radius radius of the blur
     */
    void setRadius(float radius);
};

/**
 * Intrinsic for applying a color matrix to allocations. This has the
 * same effect as loading each element and converting it to a
 * F32_N, multiplying the result by the 4x4 color matrix
 * as performed by rsMatrixMultiply() and writing it to the output
 * after conversion back to U8_N or F32_N.
 */
class ScriptIntrinsicColorMatrix : public ScriptIntrinsic {
 private:
    ScriptIntrinsicColorMatrix(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Creates a new intrinsic.
     * @param[in] rs RenderScript context
     * @return new ScriptIntrinsicColorMatrix
     */
    static sp<ScriptIntrinsicColorMatrix> create(sp<RS> rs);
    /**
     * Applies the color matrix. Supported types are U8 and F32 with
     * vector lengths between 1 and 4.
     * @param[in] in input Allocation
     * @param[out] out output Allocation
     */
    void forEach(sp<Allocation> in, sp<Allocation> out);
    /**
     * Set the value to be added after the color matrix has been
     * applied. The default value is {0, 0, 0, 0}.
     * @param[in] add float[4] of values
     */
    void setAdd(float* add);

    /**
     * Set the color matrix which will be applied to each cell of the
     * image. The alpha channel will be copied.
     *
     * @param[in] m float[9] of values
     */
    void setColorMatrix3(float* m);
    /**
     * Set the color matrix which will be applied to each cell of the
     * image.
     *
     * @param[in] m float[16] of values
     */
    void setColorMatrix4(float* m);
    /**
     * Set a color matrix to convert from RGB to luminance. The alpha
     * channel will be a copy.
     */
    void setGreyscale();
    /**
     * Set the matrix to convert from RGB to YUV with a direct copy of
     * the 4th channel.
     */
    void setRGBtoYUV();
    /**
     * Set the matrix to convert from YUV to RGB with a direct copy of
     * the 4th channel.
     */
    void setYUVtoRGB();
};

/**
 * Intrinsic for applying a 3x3 convolve to an allocation.
 */
class ScriptIntrinsicConvolve3x3 : public ScriptIntrinsic {
 private:
    ScriptIntrinsicConvolve3x3(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported types U8 and F32 with vector lengths between 1 and
     * 4. The default convolution kernel is the identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicConvolve3x3
     */
    static sp<ScriptIntrinsicConvolve3x3> create(sp<RS> rs, sp<const Element> e);
    /**
     * Sets input for intrinsic.
     * @param[in] in input Allocation
     */
    void setInput(sp<Allocation> in);
    /**
     * Launches the intrinsic.
     * @param[in] out output Allocation
     */
    void forEach(sp<Allocation> out);
    /**
     * Sets convolution kernel.
     * @param[in] v float[9] of values
     */
    void setCoefficients(float* v);
};

/**
 * Intrinsic for applying a 5x5 convolve to an allocation.
 */
class ScriptIntrinsicConvolve5x5 : public ScriptIntrinsic {
 private:
    ScriptIntrinsicConvolve5x5(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported types U8 and F32 with vector lengths between 1 and
     * 4. The default convolution kernel is the identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicConvolve5x5
     */
    static sp<ScriptIntrinsicConvolve5x5> create(sp<RS> rs, sp<const Element> e);
    /**
     * Sets input for intrinsic.
     * @param[in] in input Allocation
     */
    void setInput(sp<Allocation> in);
    /**
     * Launches the intrinsic.
     * @param[in] out output Allocation
     */
    void forEach(sp<Allocation> out);
    /**
     * Sets convolution kernel.
     * @param[in] v float[25] of values
     */
    void setCoefficients(float* v);
};

/**
 * Intrinsic for computing a histogram.
 */
class ScriptIntrinsicHistogram : public ScriptIntrinsic {
 private:
    ScriptIntrinsicHistogram(sp<RS> rs, sp<const Element> e);
    sp<Allocation> mOut;
 public:
    /**
     * Create an intrinsic for calculating the histogram of an uchar
     * or uchar4 image.
     *
     * Supported elements types are U8_4, U8_3, U8_2, and U8.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for inputs
     *
     * @return ScriptIntrinsicHistogram
     */
    static sp<ScriptIntrinsicHistogram> create(sp<RS> rs);
    /**
     * Set the output of the histogram.  32 bit integer types are
     * supported.
     *
     * @param[in] aout The output allocation
     */
    void setOutput(sp<Allocation> aout);
    /**
     * Set the coefficients used for the dot product calculation. The
     * default is {0.299f, 0.587f, 0.114f, 0.f}.
     *
     * Coefficients must be >= 0 and sum to 1.0 or less.
     *
     * @param[in] r Red coefficient
     * @param[in] g Green coefficient
     * @param[in] b Blue coefficient
     * @param[in] a Alpha coefficient
     */
    void setDotCoefficients(float r, float g, float b, float a);
    /**
     * Process an input buffer and place the histogram into the output
     * allocation. The output allocation may be a narrower vector size
     * than the input. In this case the vector size of the output is
     * used to determine how many of the input channels are used in
     * the computation. This is useful if you have an RGBA input
     * buffer but only want the histogram for RGB.
     *
     * 1D and 2D input allocations are supported.
     *
     * @param[in] ain The input image
     */
    void forEach(sp<Allocation> ain);
    /**
     * Process an input buffer and place the histogram into the output
     * allocation. The dot product of the input channel and the
     * coefficients from 'setDotCoefficients' are used to calculate
     * the output values.
     *
     * 1D and 2D input allocations are supported.
     *
     * @param ain The input image
     */
    void forEach_dot(sp<Allocation> ain);
};

/**
 * Intrinsic for applying a per-channel lookup table. Each channel of
 * the input has an independant lookup table. The tables are 256
 * entries in size and can cover the full value range of U8_4.
 **/
class ScriptIntrinsicLUT : public ScriptIntrinsic {
 private:
    sp<Allocation> LUT;
    bool mDirty;
    unsigned char mCache[1024];
    void setTable(unsigned int offset, unsigned char base, unsigned int length, unsigned char* lutValues);
    ScriptIntrinsicLUT(sp<RS> rs, sp<const Element> e);

 public:
    /**
     * Supported elements types are U8_4.
     *
     * The defaults tables are identity.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for intputs and outputs
     *
     * @return ScriptIntrinsicLUT
     */
    static sp<ScriptIntrinsicLUT> create(sp<RS> rs, sp<const Element> e);
    /**
     * Invoke the kernel and apply the lookup to each cell of ain and
     * copy to aout.
     *
     * @param[in] ain Input allocation
     * @param[in] aout Output allocation
     */
    void forEach(sp<Allocation> ain, sp<Allocation> aout);
    /**
     * Sets entries in LUT for the red channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setRed(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the green channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setGreen(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the blue channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setBlue(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the alpha channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setAlpha(unsigned char base, unsigned int length, unsigned char* lutValues);
    virtual ~ScriptIntrinsicLUT();
};

/**
 * Intrinsic for converting an Android YUV buffer to RGB.
 *
 * The input allocation should be supplied in a supported YUV format
 * as a YUV element Allocation. The output is RGBA; the alpha channel
 * will be set to 255.
 */
class ScriptIntrinsicYuvToRGB : public ScriptIntrinsic {
 private:
    ScriptIntrinsicYuvToRGB(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Create an intrinsic for converting YUV to RGB.
     *
     * Supported elements types are U8_4.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for output
     *
     * @return ScriptIntrinsicYuvToRGB
     */
    static sp<ScriptIntrinsicYuvToRGB> create(sp<RS> rs, sp<const Element> e);
    /**
     * Set the input YUV allocation.
     *
     * @param[in] ain The input allocation.
     */
    void setInput(sp<Allocation> in);

    /**
     * Convert the image to RGB.
     *
     * @param[in] aout Output allocation. Must match creation element
     *                 type.
     */
    void forEach(sp<Allocation> out);

};

/**
 * Sampler object that defines how Allocations can be read as textures
 * within a kernel. Samplers are used in conjunction with the rsSample
 * runtime function to return values from normalized coordinates.
 *
 * Any Allocation used with a Sampler must have been created with
 * RS_ALLOCATION_USAGE_GRAPHICS_TEXTURE; using a Sampler on an
 * Allocation that was not created with
 * RS_ALLOCATION_USAGE_GRAPHICS_TEXTURE is undefined.
 **/
 class Sampler : public BaseObj {
 private:
    Sampler(sp<RS> rs, void* id);
    RsSamplerValue mMin;
    RsSamplerValue mMag;
    RsSamplerValue mWrapS;
    RsSamplerValue mWrapT;
    RsSamplerValue mWrapR;
    float mAniso;

 public:
    /**
     * Creates a non-standard Sampler.
     * @param[in] rs RenderScript context
     * @param[in] min minification
     * @param[in] mag magnification
     * @param[in] wrapS S wrapping mode
     * @param[in] wrapT T wrapping mode
     * @param[in] anisotropy anisotropy setting
     */
    static sp<Sampler> create(sp<RS> rs, RsSamplerValue min, RsSamplerValue mag, RsSamplerValue wrapS, RsSamplerValue wrapT, float anisotropy);

    /**
     * @return minification setting for the sampler
     */
    RsSamplerValue getMinification();
    /**
     * @return magnification setting for the sampler
     */
    RsSamplerValue getMagnification();
    /**
     * @return S wrapping mode for the sampler
     */
    RsSamplerValue getWrapS();
    /**
     * @return T wrapping mode for the sampler
     */
    RsSamplerValue getWrapT();
    /**
     * @return anisotropy setting for the sampler
     */
    float getAnisotropy();

    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_NEAREST(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_LINEAR(sp<RS> rs);
    /**
     * Retrieve a sampler with mag set to linear, min linear mipmap linear, and
     * wrap modes set to clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_LINEAR_MIP_LINEAR(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_NEAREST(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_LINEAR(sp<RS> rs);
    /**
     * Retrieve a sampler with mag set to linear, min linear mipmap linear, and
     * wrap modes set to wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_LINEAR_MIP_LINEAR(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_NEAREST(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_LINEAR(sp<RS> rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_LINEAR_MIP_LINEAR(sp<RS> rs);

};

class Byte2 {
 public:
  int8_t x, y;

  Byte2(int8_t initX, int8_t initY)
    : x(initX), y(initY) {}
  Byte2() : x(0), y(0) {}
};

class Byte3 {
 public:
  int8_t x, y, z;

  Byte3(int8_t initX, int8_t initY, int8_t initZ)
    : x(initX), y(initY), z(initZ) {}
  Byte3() : x(0), y(0), z(0) {}
};

class Byte4 {
 public:
  int8_t x, y, z, w;

  Byte4(int8_t initX, int8_t initY, int8_t initZ, int8_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Byte4() : x(0), y(0), z(0), w(0) {}
};

class UByte2 {
 public:
  uint8_t x, y;

  UByte2(uint8_t initX, uint8_t initY)
    : x(initX), y(initY) {}
  UByte2() : x(0), y(0) {}
};

class UByte3 {
 public:
  uint8_t x, y, z;

  UByte3(uint8_t initX, uint8_t initY, uint8_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UByte3() : x(0), y(0), z(0) {}
};

class UByte4 {
 public:
  uint8_t x, y, z, w;

  UByte4(uint8_t initX, uint8_t initY, uint8_t initZ, uint8_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UByte4() : x(0), y(0), z(0), w(0) {}
};

class Short2 {
 public:
  short x, y;

  Short2(short initX, short initY)
    : x(initX), y(initY) {}
  Short2() : x(0), y(0) {}
};

class Short3 {
 public:
  short x, y, z;

  Short3(short initX, short initY, short initZ)
    : x(initX), y(initY), z(initZ) {}
  Short3() : x(0), y(0), z(0) {}
};

class Short4 {
 public:
  short x, y, z, w;

  Short4(short initX, short initY, short initZ, short initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Short4() : x(0), y(0), z(0), w(0) {}
};

class UShort2 {
 public:
  uint16_t x, y;

  UShort2(uint16_t initX, uint16_t initY)
    : x(initX), y(initY) {}
  UShort2() : x(0), y(0) {}
};

class UShort3 {
 public:
  uint16_t x, y, z;

  UShort3(uint16_t initX, uint16_t initY, uint16_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UShort3() : x(0), y(0), z(0) {}
};

class UShort4 {
 public:
  uint16_t x, y, z, w;

  UShort4(uint16_t initX, uint16_t initY, uint16_t initZ, uint16_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UShort4() : x(0), y(0), z(0), w(0) {}
};

class Int2 {
 public:
  int x, y;

  Int2(int initX, int initY)
    : x(initX), y(initY) {}
  Int2() : x(0), y(0) {}
};

class Int3 {
 public:
  int x, y, z;

  Int3(int initX, int initY, int initZ)
    : x(initX), y(initY), z(initZ) {}
  Int3() : x(0), y(0), z(0) {}
};

class Int4 {
 public:
  int x, y, z, w;

  Int4(int initX, int initY, int initZ, int initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Int4() : x(0), y(0), z(0), w(0) {}
};

class UInt2 {
 public:
  uint32_t x, y;

  UInt2(uint32_t initX, uint32_t initY)
    : x(initX), y(initY) {}
  UInt2() : x(0), y(0) {}
};

class UInt3 {
 public:
  uint32_t x, y, z;

  UInt3(uint32_t initX, uint32_t initY, uint32_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UInt3() : x(0), y(0), z(0) {}
};

class UInt4 {
 public:
  uint32_t x, y, z, w;

  UInt4(uint32_t initX, uint32_t initY, uint32_t initZ, uint32_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UInt4() : x(0), y(0), z(0), w(0) {}
};

class Long2 {
 public:
  int64_t x, y;

  Long2(int64_t initX, int64_t initY)
    : x(initX), y(initY) {}
  Long2() : x(0), y(0) {}
};

class Long3 {
 public:
  int64_t x, y, z;

  Long3(int64_t initX, int64_t initY, int64_t initZ)
    : x(initX), y(initY), z(initZ) {}
  Long3() : x(0), y(0), z(0) {}
};

class Long4 {
 public:
  int64_t x, y, z, w;

  Long4(int64_t initX, int64_t initY, int64_t initZ, int64_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Long4() : x(0), y(0), z(0), w(0) {}
};

class ULong2 {
 public:
  uint64_t x, y;

  ULong2(uint64_t initX, uint64_t initY)
    : x(initX), y(initY) {}
  ULong2() : x(0), y(0) {}
};

class ULong3 {
 public:
  uint64_t x, y, z;

  ULong3(uint64_t initX, uint64_t initY, uint64_t initZ)
    : x(initX), y(initY), z(initZ) {}
  ULong3() : x(0), y(0), z(0) {}
};

class ULong4 {
 public:
  uint64_t x, y, z, w;

  ULong4(uint64_t initX, uint64_t initY, uint64_t initZ, uint64_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  ULong4() : x(0), y(0), z(0), w(0) {}
};

class Float2 {
 public:
  float x, y;

  Float2(float initX, float initY)
    : x(initX), y(initY) {}
  Float2() : x(0), y(0) {}
};

class Float3 {
 public:
  float x, y, z;

  Float3(float initX, float initY, float initZ)
    : x(initX), y(initY), z(initZ) {}
  Float3() : x(0.f), y(0.f), z(0.f) {}
};

class Float4 {
 public:
  float x, y, z, w;

  Float4(float initX, float initY, float initZ, float initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
};

class Double2 {
 public:
  double x, y;

  Double2(double initX, double initY)
    : x(initX), y(initY) {}
  Double2() : x(0), y(0) {}
};

class Double3 {
 public:
  double x, y, z;

  Double3(double initX, double initY, double initZ)
    : x(initX), y(initY), z(initZ) {}
  Double3() : x(0), y(0), z(0) {}
};

class Double4 {
 public:
  double x, y, z, w;

  Double4(double initX, double initY, double initZ, double initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Double4() : x(0), y(0), z(0), w(0) {}
};

}

}

#endif
