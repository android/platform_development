/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef UNIQUE_PTR_H_included
#define UNIQUE_PTR_H_included

// Default deleter for pointer types.
template <typename T>
struct DefaultDelete {
    enum { type_must_be_complete = sizeof(T) };
    DefaultDelete() {}
    void operator()(T* p) const {
        delete p;
    }
};

// Default deleter for array types.
template <typename T>
struct DefaultDelete<T[]> {
    enum { type_must_be_complete = sizeof(T) };
    void operator()(T* p) const {
        delete[] p;
    }
};

// A smart pointer that deletes the given pointer on destruction.
// Equivalent to C++0x's std::unique_ptr (a combination of boost::scoped_ptr
// and boost::scoped_array).
// Named to be in keeping with Android style but also to avoid
// collision with any other implementation, until we can switch over
// to unique_ptr.
// Use thus:
//   UniquePtr<C> c(new C);
template <typename T, typename D = DefaultDelete<T> >
class UniquePtr {
public:
    // Construct a new UniquePtr, taking ownership of the given raw pointer.
    explicit UniquePtr(T* ptr = nullptr) : mPtr(ptr) { }

    UniquePtr(UniquePtr<T, D>&& that) {
      mPtr = that.mPtr;
      that.mPtr = nullptr;
    }

    ~UniquePtr() {
        reset();
    }

    // Accessors.
    T& operator*() const { return *mPtr; }
    T* operator->() const { return mPtr; }
    T* get() const { return mPtr; }

    // Returns the raw pointer and hands over ownership to the caller.
    // The pointer will not be deleted by UniquePtr.
    T* release() __attribute__((warn_unused_result)) {
        T* result = mPtr;
        mPtr = nullptr;
        return result;
    }

    // Takes ownership of the given raw pointer.
    // If this smart pointer previously owned a different raw pointer, that
    // raw pointer will be freed.
    void reset(T* ptr = nullptr) {
        if (ptr != mPtr) {
            D()(mPtr);
            mPtr = ptr;
        }
    }

private:
    // The raw pointer.
    T* mPtr;

    // Comparing unique pointers is probably a mistake, since they're unique.
    template <typename T2> bool operator==(const UniquePtr<T2>& p) const = delete;
    template <typename T2> bool operator!=(const UniquePtr<T2>& p) const = delete;

    // Disallow copy and assignment.
    UniquePtr(const UniquePtr&) = delete;
    void operator=(const UniquePtr&) = delete;
};

// Partial specialization for array types. Like std::unique_ptr, this removes
// operator* and operator-> but adds operator[].
template <typename T, typename D>
class UniquePtr<T[], D> {
public:
    explicit UniquePtr(T* ptr = NULL) : mPtr(ptr) {
    }
    UniquePtr(UniquePtr<T, D>&& that) {
      mPtr = that.mPtr;
      that.mPtr = nullptr;
    }

    ~UniquePtr() {
        reset();
    }

    T& operator[](size_t i) const {
        return mPtr[i];
    }
    T* get() const { return mPtr; }

    T* release() __attribute__((warn_unused_result)) {
        T* result = mPtr;
        mPtr = NULL;
        return result;
    }

    void reset(T* ptr = NULL) {
        if (ptr != mPtr) {
            D()(mPtr);
            mPtr = ptr;
        }
    }

private:
    T* mPtr;

    // Disallow copy and assignment.
    UniquePtr(const UniquePtr&) = delete;
    void operator=(const UniquePtr&) = delete;
};

#endif  // UNIQUE_PTR_H_included
