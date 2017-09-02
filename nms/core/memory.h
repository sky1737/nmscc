#pragma once

#include <nms/core/type.h>
#include <nms/core/trait.h>

#include <new>

#if !defined(_LIBCPP_NEW) && !defined(_NEW_) && !defined(_NEW)
#define NMS_NEW
#endif

#ifdef NMS_NEW
inline void* operator new     (size_t, void* p) noexcept {return p;}
inline void* operator new[]   (size_t, void* p) noexcept {return p;}
inline void  operator delete  (void*, void*) noexcept {}
inline void  operator delete[](void*, void*) noexcept {}
#endif

namespace nms
{

NMS_API void* _mnew (u64 size);
NMS_API void  _mdel (void* dat);
NMS_API void  _mzero(void* dat, u64 size);
NMS_API void  _mcpy (void* dst, const void* src, u64 size);
NMS_API void  _mmov (void* dst, const void* src, u64 size);
NMS_API int   _mcmp (const void* lhs, const void* rhs, u64 size);
NMS_API u64   msize (const void* ptr);

class EBadAlloc: public IException
{};

/* allocation */
template<class T>
T* mnew(u64 n){
    const auto size = n * sizeof(T);
    const auto ptr  = static_cast<T*>(_mnew(size));
    return ptr;
}

/* deallocation */
template<class T>
__forceinline void mdel(T* dat) {
    _mdel(dat);
}

/* fills a buffer with 0 */
template<class T>
__forceinline T*  mzero(T* dat, u64 n) {
    _mzero(dat, n * sizeof(T));
    return dat;
}

/* test one buffer to another */
template<class T>
__forceinline void mcmp(const T* dst, const T* src, u64 n) {
    _mcmp(dst, src, n * sizeof(T));
}

/* copies one buffer to another */
template<class T>
__forceinline void mcpy(T* dst, const T* src, u64 n) {
    if ($is_pod<T>) {
        _mcpy(dst, src, n * sizeof(T));
    }
    else {
        for (u64 i = 0; i < n; ++i) {
            new(&dst[i])T(src[i]);
        }
    }
}

/* moves one buffer to another */
template<class T>
__forceinline void mmov (T* dst, T* src, u64 n) {
    if ($is_pod<T>) {
        _mmov(dst, src, n * sizeof(T));
    }
    else {
        for (u64 i = 0u; i < n; ++i) {
            new(&dst[i])T(static_cast<T&&>(src[i]));
        }
    }
}

}
