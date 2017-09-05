#pragma once

#include <nms/core/view.h>

namespace nms::math
{

template<class T, u32 N=1>
class Array
    : public View<T, N>
    , public INocopyable
{
  public:
    using base  = View<T, N>;
    using Tsize = typename base::Tsize;
    using Tdims = typename base::Tdims;

    static const auto $rank = base::$rank;

    Array(T* data, const Tsize(&dims)[$rank])
        : base{ data, dims }
        , deleter_{}
    {}

    template<class D>
    Array(T* data, const Tsize(&dims)[N], D&& deleter)
        : base{ data, dims }
        , deleter_(fwd<D>(deleter))
    {}

    explicit Array(const Tsize(&dims)[N])
        : base(nullptr, dims)
    {
        const auto n= base::count();
        if (n != 0) {
            auto dat    = mnew<T>(n);
            base::data_ = dat;
            deleter_    = delegate<void()>([=] { mdel(dat); });
        }
    }

    virtual ~Array() {
        // try delete
        if (base::data_ != nullptr) {
            if (deleter_) {
                deleter_();
            }
            base::data_ = nullptr;
        }
    }

    Array()
        : base{}
    {}

    Array(Array&& rhs) noexcept
        : base(static_cast<base&&>(rhs))
        , deleter_(move(rhs.deleter_))
    {
        rhs.data_ = nullptr;
    }

    Array& operator=(Array&& rhs) noexcept {
        base::operator  = (static_cast<base&&>(rhs));
        deleter_        = move(rhs.deleter_);
        return *this;
    }

    auto dup() const {
        Array tmp(base::size());
        tmp <<= *this;
        return tmp;
    }

    Array& resize(const u32(&newlen)[base::$rank]) {
        const auto oldlen = base::size();

        if (oldlen == Tdims{ newlen }) {
            return *this;
        }

        Array tmp(newlen);
        *this = move(tmp);

        return *this;
    }

    Array& clear() {
        Array tmp;
        swap(*this, tmp);
        return *this;
    }

#pragma region save/load
    void save(io::File& file) const {
        return saveFile(file);
    }

    static Array load(const io::File& file) {
        return loadFile(file);
    }

    void save(const io::Path& path) const {
        return savePath<io::File>(path);
    }

    static Array load(const io::Path& path) {
        return loadPath<io::File>(path);
    }
#pragma endregion

protected:
    Array(const Array& rhs)
        : Array(rhs.size()) {
        *this <<= rhs;
    }

private:
    delegate<void()>    deleter_;

    template<class File>
    void saveFile(File& file) const {
        const auto info = base::info();
        const auto size = base::size();

        file.write(&info, 1);
        file.write(&size, 1);
        file.write(base::data(), base::count());
    }

    template<class File>
    static Array loadFile(const File& file) {
        u8x4        info;
        Vec<u32, N> size;

        file.read(&info, 1);
        file.read(&size, 1);
        if (info != base::info()) {
            NMS_THROW(EBadType{});
        }

        Array tmp(size);
        file.read(tmp.data(), tmp.count());
        return tmp;
    }

    template<class File, class Path>
    void savePath(const Path& path) const {
        File file(path, File::Write);
        saveFile(file);
    }

    template<class File, class Path>
    static Array loadPath(const Path& path) {
        File file(path, File::Read);
        return loadFile(file);
    }
};

}
