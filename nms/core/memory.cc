#include <nms/core/memory.h>
#include <nms/test.h>
#include <nms/util/stacktrace.h>

#ifdef NMS_OS_WINDOWS
extern "C" {
    using namespace nms;

    enum {
        PROT_READ      = 0x04,
        PROT_WRITE     = 0x04,

        PROT_COMMIT    = 0x8000000,
        PROT_RESERVE   = 0x4000000
    };

    enum {
        MAP_SHARED = 0x1
    };

    /*!
     * Microsoft Memory Management Functions
     * https://msdn.microsoft.com/en-us/library/aa366781(v=vs.85).aspx
     */
    int   CloseHandle(void* handle);
    void* CreateFileMappingA(void* hFile, void* attributes, i32 flProtect, u32 dwMaximumSizeHigh, u32 dwMaximumSizeLow, const char* lpName);
    void* MapViewOfFile(void* hFileMappingObject, u32 dwDesiredAccess, u32 dwFileOffsetHigh, u32 dwFileOffsetLow, u64 dwNumberOfBytesToMap);
    int   UnmapViewOfFile(void* lpBaseAddress);
    void* VirtualAlloc(void* addr, size_t size, i32 type, i32 prot);
    void* VirtualFree(void* addr, size_t size, i32 type);

    static void* mmap(void* base, u64 size, int prot, int flags, int fid, u64 offset) {
        (void)base;
        (void)flags;

        // map
        const u32 size_high = (size >> 32);
        const u32 size_low = (size << 32) >> 32;

        auto hfile = reinterpret_cast<void*>(_get_osfhandle(fid));
        auto hmmap = CreateFileMappingA(hfile, nullptr, prot, size_high, size_low, nullptr);

        // view
        const u32 offset_high = (offset >> 32);
        const u32 offset_low = (offset << 32) >> 32;
        const u32 file_map_write = 0x0002;
        auto ptr = MapViewOfFile(hmmap, file_map_write, offset_high, offset_low, size);
        if (ptr == nullptr) {
            return nullptr;
        }

        return ptr;
    }

    static int munmap(void* ptr, size_t size) {
        (void)size;
        auto ret = ::UnmapViewOfFile(ptr);
        return ret == 0 ? 1 : 0;
    }

    static void muse(void *ptr, size_t size) {
        const auto mem_commit       = 0x00001000;
        const auto mem_large_pages  = 0x20000000;   // 2MB
        ::VirtualAlloc(ptr, size, mem_commit | mem_large_pages, PROT_WRITE);
    }

    static void munuse(void* ptr, size_t size) {
        const auto mem_decommit = 0x4000;
#pragma warning(push)
#pragma warning(disable: 6250)
        ::VirtualFree(ptr, size, mem_decommit);
#pragma warning(pop)
    }

}
#endif


namespace nms
{

NMS_API void* _mnew(u64 size) {
    /*
     * @see http://en.cppreference.com/w/c/memory/malloc
     * if size == 0, the behavior is implementation defined.
     */
    if (size == 0) {
        return nullptr;
    }

    const auto ptr = static_cast<void**>(::malloc(size));
    if (ptr == nullptr) {
        NMS_THROW(EBadAlloc{});
    }
    return ptr;
}

NMS_API void  _mdel(void* ptr) {
    /*
     * @see: http://en.cppreference.com/w/c/memory/free
     * if ptr == nullptr, ::free do nothing.
     */
    ::free(ptr);
}

NMS_API void _mzero(void* dat, u64 size) {
    ::memset(dat, 0, size);
}

NMS_API int  _mcmp(const void* dst, const void* src, u64 size) {
    return ::memcmp(dst, src, size);
}


NMS_API void  _mcpy(void* dst, const void* src, u64 size) {
    ::memcpy(dst, src, size);
}

NMS_API void  _mmov(void* dst, const void* src, u64 size) {
    ::memmove(dst, src, size);
}

NMS_API u64 msize(const void* ptr) {
#if     defined(NMS_OS_WINDOWS)
    const auto mem_size = ::_msize(const_cast<void*>(ptr));
#elif   defined(NMS_OS_APPLE)
    const auto mem_size = ::malloc_size(ptr);
#elif   defined(NMS_OS_UNIX)
    const auto mem_size = ::malloc_usable_size(const_cast<void*>(ptr));
#endif
    return mem_size;
}


NMS_API void* mmap(int fid, u64 size) {
    ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fid, 0);
    return nullptr;
}

NMS_API void munmap(void* ptr, u64 size) {
    ::munmap(ptr, size);
}

NMS_API void muse(void* base, u64 size) {
    (void)base;
    (void)size;
#ifdef NMS_OS_WINDOWS
    ::muse(base, size);
#endif
}

NMS_API void munuse(void* base, u64 size) {
    (void)base;
    (void)size;
#ifdef NMS_OS_WINDOWS
    ::munuse(base, size);
#endif
}

}


#pragma region unittest
namespace nms
{

nms_test(memory) {
    struct Block
    {
        Block()
            : x(1), y(2.0)
        {}
    
        i64 x;
        f64 y;
        char c[24];
    };
    const u32 count = 1024;
    auto ptrs = new Block*[count];

    for (auto loop = 0; loop < 2; ++loop) {
        auto t0 = clock();
        for (auto i = 0u; i < count; ++i) {
            auto p = static_cast<Block*>(::malloc(sizeof(Block)));
            ptrs[i] = p;
        }
        auto t1 = clock();

        for (auto i = 0u; i < count; ++i) {
            free(ptrs[i]);
        }
        auto t2 = clock();

        if (loop > 0) {
            io::log::info("nms.memory: malloc {}, free {}", t1 - t0, t2 - t1);
        }
    }

    for (auto loop = 0; loop < 2; ++loop) {
        auto t0 = clock();
        for (auto i = 0u; i < count; ++i) {
            auto p = new Block();
            ptrs[i] = p;
        }
        auto t1 = clock();
        for (u32 i = 0; i < count; ++i) {
            delete ptrs[i];
        }
        auto t2 = clock();
        if (loop > 0) {
            io::log::info("nms.memory: new    {}, del  {}", t1 - t0, t2 - t1);
        }
    }

    for (auto loop = 0; loop < 2; ++loop) {
        auto t0 = clock();
        for (auto i = 0u; i < count; ++i) {
            auto p = mnew<Block>(1);
            new(p)Block();
            ptrs[i] = p;
        }
        auto t1 = clock();
        for (u32 i = 0; i < count; ++i) {
            mdel(ptrs[i]);
        }
        auto t2 = clock();
        if (loop > 0) {
            io::log::info("nms.memory: mnew   {}, mdel {}", t1 - t0, t2 - t1);
        }
    }

    delete[] ptrs;
}

nms_test(mmap) {
    
}

}
#pragma endregion
