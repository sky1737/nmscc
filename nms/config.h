
#ifndef _NMS_CONFIG_H_
#define _NMS_CONFIG_H_

/* check Operating System */
#if defined(_WIN32)
#   define NMS_OS_WINDOWS                   // check if os == windows
#endif

#if defined(__APPLE__)
#   define NMS_OS_APPLE                     // check if os == macos
#endif

#if defined(__unix__)
#   define NMS_OS_UNIX                      // check if os == linux
#endif

#if defined(__linux) || defined(__linux)
#   define NMS_OS_LINUX                     // check if os == unix
#endif

/* check compilier */
#if   defined(__clang__)
#   define NMS_CC_CLANG             // check if compiler == clang
#   define __forceinline inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#   define NMS_CC_MSVC              // check if compiler == msvc
#   define __PRETTY_FUNCTION__ __FUNCSIG__
#   pragma warning(disable:6326)    // E6326: potential comparison of a constant with another constant
#elif defined(__GNUC__)
#   define NMS_CC_GNUC              // check if compiler == gcc
#   define __forceinline inline __attribute__((always_inline))
#endif

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

#ifndef NMS_ABI
#   define NMS_ABI extern "C" NMS_API
#endif
#pragma endregion

#include <nms/config/stdc.h>
#include <nms/config/posix.h>

#endif  // _NMS_CONFIG_H_
