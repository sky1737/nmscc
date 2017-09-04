#pragma once

#ifdef NMS_BUILD

#ifdef NMS_OS_WINDOWS
#define _CRT_NONSTDC_NO_WARNINGS     1
#define _CRT_SECURE_NO_WARNINGS      1
#define _CRT_OBSOLETE_NO_DEPRECATE   1
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

// microsoft CRT
#ifdef NMS_OS_WINDOWS
#include <corecrt.h>
#include <corecrt_io.h>
#include <corecrt_malloc.h>
#include <corecrt_memory.h>
#include <corecrt_share.h>
#endif

#ifdef NMS_OS_APPLE
#include <malloc/malloc.h>
#include <mach-o/dyld.h>
#endif

#if defined(NMS_OS_UNIX) && !defined(NMS_OS_APPLE)
#include <malloc.h>
#endif


// c++11 threads
#if defined(NMS_OS_WINDOWS)
using thrd_t = struct thrd_st*;
using mtx_t  = struct mtx_st*;
using cnd_t  = struct cnd_st*;
#elif defined(NMS_OS_MACOS)
#include <pthread.h>
#include <semaphore.h>

using thrd_t = struct pthread_st*;

struct mtx_t {
    long sig;
    char opaque[56];
};

struct cnd_t {
    long sig;
    char opaque[40];
};
#else
#include <pthread.h>
#include <semaphore.h>
using thrd_t = pthread_t;
using mtx_t  = pthread_mutex_t;
using cnd_t  = pthread_cond_t;
#endif

#endif
