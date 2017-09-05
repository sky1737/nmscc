
#ifndef _NMS_CONFIG_H_
#define _NMS_CONFIG_H_

/* check Operating System */
#if defined(_WIN32)
#   define NMS_OS_WINDOWS                   // check if os == windows
#elif defined(__unix__) || defined(__APPLE__)
#   define NMS_OS_UNIX                      // check if os == unix
#else
#   error "unknow system"
#endif

#if __APPLE__
#   define NMS_OS_APPLE                     // check if os == macos
#endif

#ifdef __linux
#   define NMS_OS_LINUX                     // check if os == linux
#endif

/* check compilier */
#if _MSC_VER
#   define NMS_CC_MSVC                      // check if compiler == msvc
#   define __PRETTY_FUNCTION__ __FUNCSIG__
#   pragma warning(disable:6326)            // E6326: potential comparison of a constant with another constant
#endif

#ifdef __clang__
#   define NMS_CC_CLANG                     // check if compiler == clang

#endif

#ifdef __GNUC__
#   define NMS_CC_GNUC                      // check if compiler == gcc
#endif

/* define: __forceinline */
#ifdef NMS_CC_GNUC
#   define __forceinline inline __attribute__((always_inline))
#endif

/* define: NMS_API */
#ifndef NMS_API
#if defined(NMS_CC_MSVC)
#if defined NMS_BUILD
#   define NMS_API __declspec(dllexport)
#else
#   define NMS_API __declspec(dllimport)
#endif
#elif defined(NMS_CC_CLANG)
#   define NMS_API __attribute__((visibility("default")))
#else
#   define NMS_API
#endif
#endif

/* define: NMS_ABI */
#ifndef NMS_ABI
#   define NMS_ABI extern "C" NMS_API
#endif

#include <nms/config/stdc.h>
#include <nms/config/posix.h>

#endif  // _NMS_CONFIG_H_
