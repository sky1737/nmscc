#pragma once

#include <nms/core/view.h>
#include <nms/core/memory.h>

namespace nms
{

namespace io
{
class File;
class Path;
}

template<class Tdata, u32 Icapicity, bool = $is_pod<Tdata> >
struct _ListBuff;

#pragma region __ListBuff
template<class T>
struct _ListBuff<T, 0, true>
{
    const T* ptr() const {
        return nullptr;
    }

    T* ptr() {
        return nullptr;
    }
};

template<class T>
struct _ListBuff<T, 0, false>
{
    const T* ptr() const {
        return nullptr;
    }

    T* ptr() {
        return nullptr;
    }
};

template<class T, u32 N>
struct _ListBuff<T, N, true> 
{
    const T* ptr() const {
        return ptr_;
    }

    T* ptr() {
        return ptr_;
    }

    T ptr_[N] = {};
};

template<class T, u32 N>
struct alignas(T)_ListBuff<T, N, false>
{
    constexpr operator T* () const noexcept {
        return const_cast<T*>(reinterpret_cast<const T*>(buff_));
    }

    ubyte buff_[N * sizeof(T)] = {};
};
#pragma endregion

/* list */
template<class T, u32 S = 0>
class List
    : public    View<T>
    , protected _ListBuff<T, S>
{
    using base = View<T>;
    using buff = _ListBuff<T, S>;

public:
    using base::Tdata;
    using base::Tsize;
    static constexpr u32 $BlockSize = 32;           // block  size
    static constexpr u32 $BuffSize = S;     // buff   size

public:
#pragma region constructor

    /*! constructor */
    constexpr List() noexcept
        : base{ nullptr, 0 } {
        base::data_ = buff::ptr();
    }

    /* destruct */
    ~List() {
        for (Tsize i = 0u; i < base::size_; ++i) {
            base::data_[i].~T();
        }
        if (base::data_ != buff::ptr()) {
            mdel(base::data_);
        }
        base::size_ = 0;
        base::data_ = nullptr;
    }

    /* move construct */
    List(List&& rhs) noexcept
        : base(rhs), buff(rhs) {
        // check if using buff?
        if (rhs.data_ == rhs.buff::ptr()) {
            base::data_ = buff::ptr();
        }

        // clear rhs
        rhs.data_ = rhs.buff::ptr();
        rhs.size_ = 0;
    }

    /* copy construct */
    List(const List& rhs)
        : base{ nullptr, 0 } {
        appends(rhs.data(), rhs.count());
    }

#pragma endregion

#pragma region operator=
    /* move assign */
    List& operator=(List&& rhs) noexcept {
        if (this != &rhs) {
            List tmp(move(*this));
            nms::swap(static_cast<base&>(*this), static_cast<base&>(tmp));
            nms::swap(static_cast<buff&>(*this), static_cast<buff&>(tmp));
        }
        return *this;
    }

    /* copy assign */
    /* move assign */
    List& operator=(const List& rhs) noexcept {
        if (this != &rhs) {
            List tmp(rhs);
            nms::swap(*this, tmp);
        }
        return *this;
    }
#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;

    /* the number of elements that can be held in currently allocated storage */
    Tsize capicity() const noexcept {
        if (data_ == buff::ptr()) {
            return $BuffSize;
        }
        const auto mem_size = msize(base::data_);
        const auto count = (mem_size - sizeof(u32)) / sizeof(T);
        return count;
    }

#pragma endregion

#pragma region method
    /* clear data */
    List& clear() {
        List tmp;
        swap(*this, tmp);
        return *this;
    }

    /*!
     * reserves storge
     * if (newlen <= capicity()) { do nothing }
     * else { new storge is allocated }
     */
    List& reserve(Tsize newlen) {
        const auto oldcap = capicity();

        // do not need realloc
        if (newlen <= oldcap) {
            return *this;
        }

        const auto olddat = data();
        const auto oldlen = count();

        const auto newcap = (newlen + oldlen / 16 + 63) & ~63ull;
        const auto newdat = mnew<T>(newcap);

        if (oldlen > 0) {
            nms::mmov(newdat, olddat, oldlen);
            if (olddat != buff::ptr()) {
                nms::mdel(olddat);
            }
        }
        base::data_ = newdat;
        return *this;
    }

    /*!
     * append elements to the end
     */
    template<class ...U>
    List& append(U&& ...u) {
        const auto oldlen = base::count();
        const auto newlen = oldlen + 1;
        reserve(newlen);
        _append(fwd<U>(u)...);
        return *this;
    }

    /*!
    * append elements to the end
    */
    template<class ...U>
    List& appends(Tsize cnt, U&& ...u) {
        const auto oldlen = base::count();
        const auto newlen = oldlen + cnt;
        reserve(newlen);
        base::size_ = newlen;

        auto ptr = base::data();
        for (auto i = oldlen; i < newlen; ++i) {
            new(&ptr[i])T(fwd<U>(u)...);
        }
        return *this;
    }


    /*!
     * append(copy) elements to the end
     */
    template<class U>
    List& appends(const U* src, Tsize cnt) {
        auto old_cnt = base::count();
        reserve(old_cnt + cnt);
        base::size_ += cnt;

        // modify data
        auto ptr = base::data_ + old_cnt;
        for (u32 i = 0; i < cnt; ++i, ++ptr) {
            new(ptr)T(src[i]);
        }

        return *this;
    }

    /*!
     * append an element to the end
     */
    template<class U>
    List& operator+=(U&& u) {
        append(fwd<U>(u));
        return *this;
    }

protected:
    /*!
     * append elements to the end
     */
    template<class ...U>
    List& _append(U&& ...u) {
        auto ptr = base::data();
        auto idx = base::size_++;
        new(&ptr[idx])T(fwd<U>(u)...);
        return *this;
    }
#pragma endregion

#pragma region save/load
    void save(io::File& os) const;
    static List load(io::File& is);

    void save(const io::Path& path) const;
    static List load(const io::Path& path);
#pragma endregion
};


}

