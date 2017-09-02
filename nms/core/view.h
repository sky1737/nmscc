#pragma once

#include <nms/core/base.h>
#include <nms/core/trait.h>

namespace nms
{
template<class T, u32 N = 0>
struct View;

using StrView = View<const char>;

template<class Tidx, class Tsize>
constexpr auto shrinkIdx(Tidx idx, Tsize size) {
    return idx >= 0 ? Tsize(idx) : size-Tsize(0-idx);
}

#pragma region mkStride
template<class Tint, u32 N, u32 ...I>
__forceinline constexpr auto mkStride(const Tint(&size)[N], U32<I...>) {
    return Vec<Tint,N>{ iprod(Seq<I>{}, size)... };
}

template<class Tint, u32 N>
__forceinline constexpr auto mkStride(const Tint(&size)[N]) {
    return mkStride(size, Seq<N>{});
}
#pragma endregion

template<class T, u32 N>
struct View
{
#pragma region defines
    constexpr static const auto $rank = N;

    using Tdata     = T;
    using Tsize     = u32;
    using Trank     = u32;
    using Tdims     = Vec<Tsize,$rank>;

    template<class U, u32 M>
    friend struct View;
#pragma endregion

#pragma region constructors
    /* default constructor */
    __forceinline View()  = default;

    /* default destructor */
    __forceinline ~View() = default;

    /*! construct view with data, size, stride */
    __forceinline constexpr View(Tdata* data, const Tsize(&size)[$rank], const Tsize(&stride)[$rank])
        : data_{ data }, size_{ size }, stride_(stride)
    {}

    /*! construct view with data, size */
    __forceinline constexpr View(Tdata* data, const Tsize(&size)[$rank])
        : data_{ data }, size_{ size }, stride_{ mkStride(size) }
    {}

    /*! convert to const View */
    __forceinline operator View<const Tdata, $rank>() const noexcept {
        return { data_, size_, stride_ };
    }

#pragma endregion

#pragma region properties
    __forceinline static constexpr Trank rank() {
        return $rank;
    }

    /*! get data pointer */
    __forceinline Tdata* data() noexcept {
        return data_;
    }

    /*! get data pointer */
    __forceinline const Tdata* data() const noexcept {
        return data_;
    }

    /*! get n-dim size */
    __forceinline Tdims size() const noexcept {
        return size_;
    }

    /*! get n-dim stride */
    __forceinline Tdims stride() const noexcept {
        return stride_;
    }

    /*! get total elements count */
    __forceinline Tsize count() const noexcept {
        return iprod(Seq<$rank>{}, size_);
    }

    /*!
     * get idx-dim size
     * @see size
     */
    template<class Tdim>
    __forceinline constexpr Tsize size(Tdim dim) const noexcept {
        return size_[dim];
    }

    /*!
     * get idx-dim stride
     * @see stride
     */
    template<class Tdim>
    __forceinline constexpr Tsize stride(Tdim dim) const noexcept {
        return stride_[dim];
    }

    /*!
     * test if stride is default
     * @see stride
     */
    __forceinline constexpr bool isNormal() const {
        return stride_ == mkStride(size_.data_);
    }

    /*!
     * test if empty (count()==0)
     * @see count
     */
    constexpr __forceinline bool isEmpty() const {
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

#pragma region save/load
    __forceinline static u8x4 typeinfo() {
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

#pragma region offset_of
    template<u32 Idim>
    __forceinline constexpr Tsize offset_of(u32 idx) const noexcept {
        return idx * stride_[Idim];
    }

    template<u32 Idim>
    __forceinline constexpr Tsize offset_of(i32 idx) const noexcept {
        return (idx >= 0 ? idx : i32(size_[0]) + idx) * stride_[Idim];
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
    __forceinline constexpr Tsize size_of(const Tidx(&)[1]) const noexcept {
        return 0u;
    }

    template<u32 Idim, class Tidx>
    __forceinline constexpr Tsize size_of(const Tidx(&idx)[2]) const noexcept {
        return offset_of<Idim>(idx[1]) - offset_of<Idim>(idx[0]) + 1;
    }

    template<u32 ...Idim, class ...Tidx, u32 ...Isize>
    __forceinline constexpr Tdims sizes_of(U32<Idim...>, const Tidx(&...idxs)[Isize]) const noexcept {
        return {size_of<Idim>(idxs)...};
    }

    template<class ...Tidx, u32 ...Isize>
    __forceinline constexpr Tdims sizes_of(const Tidx(&...idxs)[Isize]) const noexcept {
        return sizes_of(Seq<$rank>{}, idxs...);
    }
#pragma endregion

#pragma region _slice
    /* slice */
    template<class ...Tidx, u32 ...Icnt>
    __forceinline View<Tdata, $rank> _slice(const Tidx(&...s)[Icnt]) noexcept {
        static_assert(all((Icnt <= 2)...), "unexpect array size");
        return { data_ + offsets_of(s[0]...), sizes_of(s...), stride_};
    }

    /* slice */
    template<class ...Tidx, u32 ...Icnt>
    __forceinline constexpr View<const Tdata, $rank> _slice(const Tidx(&...s)[Icnt]) const noexcept {
        static_assert(all((Icnt <= 2)...), "unexpect array size");
        return { data_ + offsets_of(s[0]...), sizes_of(s...), stride_};
    }

    /* select dim */
    template<u32 ...I>
    __forceinline constexpr View<Tdata, u32(sizeof...(I))> _select(U32<I...>) const noexcept {
        return { data_, { size_[I]... }, { stride_[I]... } };
    }
#pragma endregion
};

template<class T>
struct View<T, 0>
{
#pragma region defines
    using Tdata = T;
    using Tsize = u64;
    using Trank = u32;

    constexpr static const Trank $rank = 0;

    template<class U, u32 M>
    friend struct View;
#pragma endregion

#pragma region constructors
    /* default constructor */
    __forceinline View() = default;

    /* default destructor */
    __forceinline ~View() = default;

    /*! construct view with data, size, stride */
    __forceinline constexpr View(Tdata* data, Tsize size)
        : data_{ data }, size_{ size }
    {}

    /*! construct view with data, size, stride */
    template<u32 Isize>
    __forceinline constexpr View(Tdata (&data)[Isize])
        : data_{ data }, size_{ ($is<T, char> || $is<T, const char>) ? Isize-1: Isize } 
    {}

    /*! convert to const View */
    __forceinline operator View<const T>() const noexcept {
        return { data_, size_ };
    }

#pragma endregion

#pragma region properties
    __forceinline static constexpr Trank rank() {
        return $rank;
    }

    /*! get data pointer */
    __forceinline Tdata* data() noexcept {
        return data_;
    }

    /*! get data pointer */
    __forceinline const Tdata* data() const noexcept {
        return data_;
    }

    /*! get n-dim size */
    __forceinline Tsize size() const noexcept {
        return size_;
    }

    /*! get total elements count */
    __forceinline Tsize count() const noexcept {
        return size_;
    }

    /*!
    * test if empty (count()==0)
    * @see count
    */
    constexpr __forceinline bool isEmpty() const {
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

#pragma region slice
    /*! slice */
    template<class Tidx>
    View<Tdata> slice(Tidx first, Tidx last) noexcept {
        return { data_ + offset_of(first), offset_of(last) - offset_of(first) + 1 };
    }

    /*! slice */
    template<class Tidx>
    View<const Tdata> slice(Tidx first, Tidx last) const noexcept {
        return { data_ + offset_of(first), offset_of(last) - offset_of(first) + 1 };
    }

    /*! slice */
    template<class Tidx>
    View<Tdata> operator()(Tidx first, Tidx last) noexcept {
        return { data_ + offset_of(first), offset_of(last) - offset_of(first) + 1 };
    }

    /*! slice */
    template<class Tidx>
    View<const Tdata> operator()(Tidx first, Tidx last) const noexcept {
        return { data_ + offset_of(first), offset_of(last) - offset_of(first) + 1 };
    }
#pragma endregion

#pragma region save/load
    __forceinline static u8x4 typeinfo() {
        const auto ch =
            $is<$uint,  T> ? 'u' :
            $is<$sint,  T> ? 'i' :
            $is<$float, T> ? 'f' :
            '?';
        const auto size = sizeof(T);
        const u8x4 val = { u8('$'), u8(ch), u8('0' + size), u8('0') };
        return val;
    }

#pragma endregion

protected:
    Tdata*  data_;
    Tsize   size_;

#pragma region offset_of
    __forceinline constexpr auto offset_of(u32 idx) const noexcept {
        return idx;
    }

    __forceinline constexpr auto offset_of(u64 idx) const noexcept {
        return idx;
    }

    __forceinline constexpr auto offset_of(i32 idx) const noexcept {
        return idx >= 0 ? idx : u32(size_) + u32(-idx);
    }

    __forceinline constexpr auto offset_of(i64 idx) const noexcept {
        return idx >= 0 ? idx : u64(size_) + u64(-idx);
    }

#pragma endregion

};

#pragma region iterator
template<class T>
auto begin(View<T>& v) {
    return v.data();
}

template<class T>
auto end(View<T>& v) {
    return v.data() + v.count();
}

template<class T>
auto begin(const View<T>& v) {
    return v.data();
}

template<class T>
auto end(const View<T>& v) {
    return v.data() + v.count();
}
#pragma endregion

#pragma region mkView
template<class T, u32 N, class Tsize, class Tstride>
__forceinline View<T, N> mkView(T* ptr, const Tsize(&size)[N], const Tstride(&stride)[N]) {
    return { ptr, size, stride };
}

template<class T, u32 N, class Tsize, class Tstride>
__forceinline View<T, N> mkView(T* ptr, const Tsize(&size)[N]) {
    return { ptr, size, mkStride(size) };
}
#pragma endregion

/* view is lambda */
template<class T, u32 N>
__forceinline auto toLambda(const View<T,N>& val) {
    return val;
}

/* reshape */
template<class T, u32 N, u32 M>
__forceinline auto reshape(View<T, N> view, const u32(&new_size)[M]) {
    return mkView(view.dat(), new_size);
}

/* reshape */
template<class T, u32 N, u32 M>
__forceinline auto reshape(View<T, N> view, const u32(&new_size)[M], const u32(&new_stride)[M]) {
    return mkView(view.data(), new_size, new_stride);
}

/* premute */
template<class T, u32 N>
auto permute(const View<T, N> &view, const u32(&order)[N]) {
    u32 new_size[N];
    u32 new_stride[N];

    for (u32 i = 0; i < N; ++i) {
        new_size[i]     = view.size(order[i]);
        new_stride[i]   = view.stride(order[i]);
    }
    return mkView(const_cast<T*>(view.data()), new_size, new_stride);
}


/* check if view equals */
template<class T>
bool operator==(const View<T, 0>& a, const View<T, 0>& b) {
    if (&a == &b)               return true;
    if (a.size() != b.size())   return false;

    const auto size = a.size();
    for (u32 i = 0; i < size[0]; ++i) {
        if (a(i) != b(i)) {
            return false;
        }
    }
    return true;
}

/* check if view equals */
template<class T>
bool operator==(const View<T, 1>& a, const View<T, 1>& b) {
    if (&a == &b)               return true;
    if (a.size() != b.size())   return false;

	const auto size = a.size();
    for (u32 i = 0; i < size[0]; ++i) {
        if (a(i) != b(i)) {
            return false;
        }
    }
    return true;
}

/* check if view not equals */
template<class T, u32 N>
bool operator!=(const View<T, N>& a, View<T, N>& b) {
    return !(a == b);
}

/* check if view equals */
template<class T, u32 N>
static bool operator==(const View<T>& a, T(&b)[N]) {
    return a == View<T>{ b };
}

/* check if StrView not equals */
template<class T, u32 N>
static bool operator!=(const View<T>& a, T(&b)[N]) {
    return a != StrView{ b };
}

/* check if StrView equals */
template<class T, u32 N>
static bool operator==(T(&a)[N], View<T>& b) {
    return StrView{ a } == b;
}

/* check if View not equals */
template<class T, u32 N>
static bool operator!=(T(&a)[N], View<T>& b) {
    return StrView{ a } != b;
}

}
