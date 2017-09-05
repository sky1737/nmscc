#pragma once

#include <nms/core/base.h>
#include <nms/core/trait.h>
#include <nms/core/math.h>

namespace nms
{

template<class T, u32 N = 0>
struct View;

using StrView = View<const char>;

template<class T, u32 N>
struct View
{
#pragma region defines
    constexpr static const auto $rank = N;

    using Tdata     = T;
    using Tsize     = u32;
    using Trank     = u32;
    using Tdims     = Vec<Tsize,$rank>;
    using Tinfo     = u8x4;
    using Tview     = View;

    template<class U, u32 M>
    friend struct View;
#pragma endregion

#pragma region constructors
    /* default constructor */
    View()  = default;

    /* default destructor */
    ~View() = default;

    /*! construct view with data, size, stride */
    constexpr View(Tdata* data, const Tsize(&size)[$rank], const Tsize(&stride)[$rank])
        : data_{ data }, size_{ size }, stride_(stride)
    {}

    /*! construct view with data, size */
    constexpr View(Tdata* data, const Tsize(&size)[$rank])
        : data_{ data }, size_{ size }, stride_{ mkStride(size) }
    {}

    /*! convert to const View */
    operator View<const Tdata, $rank>() const noexcept {
        return { data_, size_, stride_ };
    }

#pragma endregion

#pragma region properties
    static constexpr Trank rank() {
        return $rank;
    }

    /*! get data pointer */
    Tdata* data() noexcept {
        return data_;
    }

    /*! get data pointer */
    const Tdata* data() const noexcept {
        return data_;
    }

    /*! get n-dim size */
    Tdims size() const noexcept {
        return size_;
    }

    /*! get n-dim stride */
    Tdims stride() const noexcept {
        return stride_;
    }

    /*! get total elements count */
    Tsize count() const noexcept {
        return iprod(Seq<$rank>{}, size_);
    }

    /*!
     * get idx-dim size
     * @see size
     */
    template<class Tdim>
    constexpr Tsize size(Tdim dim) const noexcept {
        return size_[dim];
    }

    /*!
     * get idx-dim stride
     * @see stride
     */
    template<class Tdim>
    constexpr Tsize stride(Tdim dim) const noexcept {
        return stride_[dim];
    }

    /*!
     * test if stride is default
     * @see stride
     */
    constexpr bool isNormal() const {
        return stride_ == mkStride(size_.data_);
    }

    /*!
     * test if empty (count()==0)
     * @see count
     */
    constexpr bool isEmpty() const {
        return count() == 0;
    }

#pragma endregion

#pragma region access
    /*!
     * access specified element
     */
    template<class ...Tidx>
    __forceinline const Tdata& at(Tidx ...idx) const noexcept {
        static_assert($all_is<$int,Tidx...>,    "unexpect type");
        static_assert(sizeof...(Tidx)==$rank,    "unexpect arguments count");
        return data_[offsets_of(idx...)];
    }

    /*!
     * access specified element
     */
    template<class ...Tidx>
    __forceinline Tdata& at(Tidx ...idx) noexcept {
        static_assert($all_is<$int, Tidx...>,  "unexpect type");
        static_assert(sizeof...(Tidx)==$rank,  "unexpect arguments count");
        return data_[offsets_of(idx...)];
    }

    /*!
     * access specified element
     * @param idx indexs
     * @see at
     */
    template<class ...I>
    __forceinline Tdata& operator()(I ...idx) noexcept {
        return at(idx...);
    }

    /*!
     * access specified element
     * @param idx indexs
     * @see at
     */
    template<class ...I>
    __forceinline const Tdata& operator()(I ...idx) const noexcept {
        return at(idx...);
    }

#pragma endregion

#pragma region slice
    /*! slice */
    template<class ...Tidx, u32 ...Icnt>
    View<const Tdata, Index<(Icnt>1)...>::$size> slice(const Tidx(&...ids)[Icnt]) const noexcept {
        return _slice(ids...)._select(Index<(Icnt > 1)...>{});
    }

    /*! slice */
    template<class ...Tidx, u32 ...Icnt>
    View<Tdata, Index<(Icnt>1)...>::$size>slice(const Tidx(&...idxs)[Icnt]) noexcept {
        return _slice(idxs...)._select(Index<(Icnt > 1)...>{});
    }

    /*!
     * slice the view
     * @param ids sections
     * @see slice
     */
    template<class ...Tidx, u32 ...Icnt >
    View<const Tdata, Index<(Icnt>1)...>::$size> operator()(const Tidx(&...ids)[Icnt]) const noexcept {
        return _slice(ids...)._select(Index<(Icnt > 1)...>{});
    }

    /*!
     * slice the view
     * @param ids sections
     * @see slice
     */
    template<class ...Tidx, u32 ...Icnt >
    View<Tdata, Index<(Icnt>1)...>::$size> operator()(const Tidx(&...ids)[Icnt]) noexcept {
        return _slice(ids...)._select(Index<(Icnt > 1)...>{});
    }

#pragma endregion

#pragma region methods
View permute(const u32(&order)[$rank]) const {
    Tdims   new_size;
    Tdims   new_stride;

    for (Trank i = 0; i < $rank; ++i) {
        new_size[i]     = size_(order[i]);
        new_stride[i]   = stride_(order[i]);
    }

    return { data_, new_size, new_stride};
}

template<u32 M>
View<Tdata,M> reshape(const u32(&new_size)[M]) const {
    return { data_, new_size};
}

template<u32 M>
View<Tdata,M> reshape(const u32(&new_size)[M], const u32(&new_stride)[M]) const {
    return { data_, new_size, new_stride};
}

#pragma endregion

#pragma region save/load
    static Tinfo info() {
        const auto ch =
            $is<$uint, Tdata> ? 'u' :
            $is<$sint, Tdata> ? 'i' :
            $is<$float,Tdata> ? 'f' :
            '?';

        const auto size = sizeof(Tdata);
        const u8x4 val = { u8('$'), u8(ch), u8('0' + size), u8('0' + N) };

        return val;
    }

#pragma endregion

protected:
    Tdata*  data_;
    Tdims   size_;
    Tdims   stride_;    // stride[0] = capicity

#pragma region index_of
    template<u32 Idim>
    __forceinline constexpr Tsize index_of(u32 idx) const noexcept {
        return idx;
    }

    template<u32 Idim>
    __forceinline constexpr Tsize index_of(i32 idx) const noexcept {
        return idx >= 0 ? idx : i32(size_[0]) + idx;
    }

#pragma endregion

#pragma region offset_of
    template<u32 Idim, class Tidx>
    __forceinline constexpr Tsize offset_of(Tidx idx) const noexcept {
        return index_of<Idim>(idx) * stride_[Idim];
    }

    template<u32 ...Idim, class ...Tidx>
    __forceinline constexpr Tsize offsets_of(U32<Idim...>, Tidx ...idx) const noexcept {
        return sum(offset_of<Idim>(idx)...);
    }

    template<class ...Tidx>
    __forceinline constexpr Tsize offsets_of(Tidx ...idxs) const noexcept {
        return offsets_of(Seq<$rank>{}, idxs...);
    }
#pragma endregion

#pragma region size_of
    template<u32 Idim, class Tidx>
    constexpr Tsize size_of(const Tidx(&)[1]) const noexcept {
        return 0u;
    }

    template<u32 Idim, class Tidx>
    constexpr Tsize size_of(const Tidx(&idx)[2]) const noexcept {
        return index_of<Idim>(idx[1]) - index_of<Idim>(idx[0]) + 1;
    }

    template<u32 ...Idim, class ...Tidx, u32 ...Isize>
    constexpr Tdims sizes_of(U32<Idim...>, const Tidx(&...idxs)[Isize]) const noexcept {
        return {size_of<Idim>(idxs)...};
    }

    template<class ...Tidx, u32 ...Isize>
    constexpr Tdims sizes_of(const Tidx(&...idxs)[Isize]) const noexcept {
        return sizes_of(Seq<$rank>{}, idxs...);
    }
#pragma endregion

#pragma region _slice
    /* slice */
    template<class ...Tidx, u32 ...Icnt>
    View<Tdata, $rank> _slice(const Tidx(&...s)[Icnt]) noexcept {
        static_assert(all((Icnt <= 2)...), "unexpect array size");
        return { data_ + offsets_of(s[0]...), sizes_of(s...), stride_};
    }

    /* slice */
    template<class ...Tidx, u32 ...Icnt>
    View<const Tdata, $rank> _slice(const Tidx(&...s)[Icnt]) const noexcept {
        static_assert(all((Icnt <= 2)...), "unexpect array size");
        return { data_ + offsets_of(s[0]...), sizes_of(s...), stride_};
    }

    /* select dim */
    template<u32 ...I>
    View<Tdata, u32(sizeof...(I))> _select(U32<I...>) const noexcept {
        return { data_, { size_[I]... }, { stride_[I]... } };
    }
#pragma endregion

private:
    static constexpr Tdims mkStride(const Tdims& size) {
        return mkStride(Seq<$rank>{}, size);
    }

    template<u32 ...I>
    static constexpr Tdims mkStride(U32<I...>, const Tdims& size) {
        return { iprod(Seq<I>{}, size)... };
    }
};

template<class T>
struct View<T, 0>
{
#pragma region defines
    constexpr static const u32 $rank = 1;

    using Tdata = T;
    using Tsize = u32;
    using Trank = u32;
    using Tdims = Vec<u32, $rank>;
    using Tinfo = u8x4;

    template<class U, u32 M>
    friend struct View;
#pragma endregion

#pragma region constructors
    /* default constructor */
    constexpr View() = default;

    /* default destructor */
    ~View() = default;

    /*! construct view with data, size, stride */
    constexpr View(Tdata* data, Tsize size)
        : data_{ data }, size_{ size }
    {}

    /*! construct view with data, size, stride */
    template<u32 Isize>
    constexpr View(Tdata(&data)[Isize])
        : data_{ data }, size_{ ($is<T, char> || $is<T, const char>) ? Isize - 1 : Isize } {
    }

    /*! convert to const View */
    operator View<const T>() const noexcept {
        return { data_, size_ };
    }

#pragma endregion

#pragma region properties
    static constexpr Trank rank() {
        return $rank;
    }

    /*! get data pointer */
    Tdata* data() noexcept {
        return data_;
    }

    /*! get data pointer */
    const Tdata* data() const noexcept {
        return data_;
    }

    /*! get n-dim size */
    Tdims size() const noexcept {
        return { size_ };
    }

    /*! get total elements count */
    Tsize count() const noexcept {
        return size_;
    }

    /*!
    * test if empty (count()==0)
    * @see count
    */
    constexpr bool isEmpty() const {
        return count() == 0;
    }

#pragma endregion

#pragma region access
    /*! access specified element */
    template<class Tidx>
    __forceinline const Tdata& at(Tidx idx) const noexcept {
        return data_[offset_of(idx)];
    }

    /*! access specified element */
    template<class Tidx>
    __forceinline Tdata& at(Tidx idx) noexcept {
        return data_[offset_of(idx)];
    }

    /*! access specified element */
    template<class Tidx>
    __forceinline Tdata& operator()(Tidx idx) noexcept {
        return at(idx);
    }

    /*! access specified element */
    template<class Tidx>
    __forceinline const Tdata& operator()(Tidx idx) const noexcept {
        return at(idx);
    }

    /*! access specified element */
    template<class Tidx>
    __forceinline Tdata& operator[](Tidx idx) noexcept {
        return at(idx);
    }

    /*! access specified element */
    template<class Tidx>
    __forceinline const Tdata& operator[](Tidx idx) const noexcept {
        return at(idx);
    }
#pragma endregion

#pragma region iterator
    Tdata* begin() {
        return data_;
    }

    Tdata* end() {
        return data_ + size_;
    }

    const Tdata* begin() const {
        return data_;
    }

    const Tdata* end() const {
        return data_ + size_; 
    }
#pragma endregion

#pragma region slice
    /*! slice */
    template<class Tidx>
    View<Tdata> slice(Tidx first, Tidx last) noexcept {
        const auto data = data_ + offset_of(first);
        const auto size = offset_of(last) - offset_of(first) + 1;
        return { data, size };
    }

    /*! slice */
    template<class Tidx>
    View<const Tdata> slice(Tidx first, Tidx last) const noexcept {
        const auto data = data_ + offset_of(first);
        const auto size = offset_of(last) - offset_of(first) + 1;
        return { data, size };
    }

    /*! slice */
    template<class Tidx>
    View<Tdata> operator()(Tidx first, Tidx last) noexcept {
        return slice(first, last);
    }

    /*! slice */
    template<class Tidx>
    View<const Tdata> operator()(Tidx first, Tidx last) const noexcept {
        return slice(first, last);
    }
#pragma endregion

#pragma region method
    int compare(const View<T>& b) const {
        auto& a = *this;

        if (&a == &b) {
            return 0;
        }

        const auto na = a.count();
        const auto nb = b.count();

        if (na != nb) {
            return na > nb ? +1 : -1;
        }

        for (Tsize i = 0; i < na; ++i) {
            if (a[i] != b[i]) {
                return a[i] > b[i] ? +1 : -1;
            }
        }
        return 0;
    }

    bool operator== (const View& v) const {
        return compare(v) == 0;
    }

    bool operator!= (const View& v) const {
        return compare(v) != 0;
    }

#pragma endregion

#pragma region save/load
    static u8x4 info() {
        const auto ch =
            $is<$uint, T> ? 'u' :
            $is<$sint, T> ? 'i' :
            $is<$float, T> ? 'f' :
            '?';
        const auto size = sizeof(T);
        const u8x4 val = { u8('$'), u8(ch), u8('0' + size), u8('0') };
        return val;
    }

#pragma endregion

protected:
    Tdata*  data_       = nullptr;
    Tsize   size_       = 0;
    Tsize   capacity_   = 0;

#pragma region offset_of
    __forceinline constexpr u32 offset_of(u32 idx) const noexcept {
        return idx;
    }

    __forceinline constexpr u64 offset_of(u64 idx) const noexcept {
        return idx;
    }

    __forceinline constexpr u32  offset_of(i32 idx) const noexcept {
        return idx >= 0 ? idx : u32(size_) - u32(-idx);
    }

    __forceinline constexpr u64 offset_of(i64 idx) const noexcept {
        return idx >= 0 ? idx : u64(size_) - u64(-idx);
    }

#pragma endregion

};

template<class T>
struct Scalar
{
    using Trank = u32;
    using Tsize = u32;
    using Tdata = T;
    using Tview = Scalar;

    constexpr static const auto $rank = 0;

    Scalar(const T& t)
        : t_(t) {
    }

    template<class I>
    Tsize size(I idx) const noexcept {
        return 1u;
    }

    template<class ...I>
    const T& operator()(I ...idx) const noexcept {
        return t_;
    }

protected:
    T   t_;
};

#pragma region make
template<class T, u32 N>
View<T, N> mkView(T* ptr, const u32(&size)[N], const u32(&stride)[N]) {
    return { ptr, size, stride };
}

template<class T, u32 N>
View<T, N> mkView(T* ptr, const u32(&size)[N]) {
    return { ptr, size };
}

template<class T, u32 S>
View<T> mkView(T(&data)[S]) {
    return {data};
}

#pragma endregion

}
