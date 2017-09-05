#pragma once

#include <nms/core.h>
#include <nms/io/path.h>

namespace nms
{
template<class T, u32 BookSize, u32 PageSize>
class ArrayList;
}

namespace nms::math
{
template<class T, u32 N>
class Array;
}

namespace nms::io
{

class File: public INocopyable
{
public:
    enum OpenMode
    {
        Read    = 0x0,      /// open for read, file must exists
        Write   = 0x1,      /// open for write, file will trunk.
        Append  = 0x2,      /// open for append
    };

    class Exception: public IException
    {};

    class ENotEnough: public Exception
    {};

    NMS_API File(const Path& path, OpenMode mode);
    NMS_API virtual ~File();
    

    File(File&& rhs) noexcept
        : obj_(rhs.obj_)
    {
        rhs.obj_ = nullptr;
    }

    File& operator=(File&& rhs) noexcept {
        nms::swap(obj_, rhs.obj_);
        return *this;
    }

    NMS_API u64 size() const;
    NMS_API int id()   const;

#pragma region read/write

#pragma region raw
    NMS_API void sync() const;


    u64 read(void* data, u64 size) const {
        return readRaw(data, 1, size);
    }

    u64 write(const void* buff, u64 size) {
        return writeRaw(buff, 1, size);
    }

#pragma endregion

#pragma region array/vec
    /* read data */
    template<class T>
    u64 read(T data[], u64 count) const {
        auto nread = readRaw(static_cast<void*>(data), sizeof(T), count);
        return nread;
    }

    /* write data */
    template<class T>
    u64 write(const T data[], u64 count) {
        auto nwrite = writeRaw(data, sizeof(T), count);
        return nwrite;
    }
#pragma endregion

#pragma region view<T,N>
    /* read view data */
    template<class T, u32 N>
    void read(View<T, N>& view) const {
        if (!view.isNormal()) {
            NMS_THROW(EBadType{});
        }

        read(view.data(), view.count());
    }

    /* write view data */
    template<class T, u32 N>
    void write(const View<T, N>& view) {
        if (!view.isNormal()) {
            NMS_THROW(EBadType{});
        }
        write(view.data(), view.count());
    }

protected:
#ifndef NMS_BUILD
    struct fid_t;
#else
    using fid_t = ::FILE;
#endif

    fid_t*  obj_;   // the FILE* object

    NMS_API u64 readRaw (void*       buffer, u64 size, u64 count) const;
    NMS_API u64 writeRaw(const void* buffer, u64 size, u64 count);
};

using DatFile = File;
using BinFile = File;

class TxtFile
    : protected File
{
protected:
    using base = File;

public:
    NMS_API TxtFile(const Path& path, File::OpenMode mode);
    NMS_API virtual ~TxtFile();

    using base::OpenMode;
    using base::size;
    using base::sync;

    u64 read(char* u8_buf, u64 size) const {
        return _read(u8_buf, size);
    }

    u64 write(StrView u8_str) {
        return _write(u8_str.data(), u8_str.count());
    }

    template<class ...U>
    u64 write(StrView fmt, const U& ...args) {
        auto s = format(fmt, args...);
        return write(s);
    }

private:
    NMS_API u64 _read(char* u8_buf, u64 size) const;
    NMS_API u64 _write(const char* u8_buf, u64 size);
};

NMS_API u64   fsize(const Path& path);
NMS_API u64   fsize(int fid);

NMS_API String loadString(const Path& path);

}
