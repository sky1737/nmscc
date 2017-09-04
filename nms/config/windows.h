#pragma once

#ifdef NMS_OS_WINDOWS

#ifdef NMS_BUILD
#define _CRT_NONSTDC_NO_WARNINGS     1
#define _CRT_SECURE_NO_WARNINGS      1
#define _CRT_OBSOLETE_NO_DEPRECATE   1

// stdc
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

// posix
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <process.h>

// system
#include <sys/stat.h>

// stdc++
#include <typeinfo>

#define NMS_API __declspec(dllexport)

using stat_t = struct ::_stat64;

static inline int fstat(int fd, stat_t* st) {
    return ::_fstat64(fd, st);
}

static inline int stat(const char* path, stat_t* st) {
    return ::_stat64(path, st);
}

#endif

using thrd_t = void*;
using mtx_t  = void*;
using cnd_t  = void*;

#endif
