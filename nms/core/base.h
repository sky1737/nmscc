#pragma once

#include <nms/config.h>

namespace nms
{

#pragma region core types
using   i8          = int8_t;       using   u8      = uint8_t;
using   i16         = int16_t;      using   u16     = uint16_t;
using   i32         = int32_t;      using   u32     = uint32_t;
using   i64         = int64_t;      using   u64     = uint64_t;

using   byte        = i8;
using   ubyte       = u8;
using   f32         = float;
using   f64         = double;

using   uchar       = unsigned char;
using   ushort      = unsigned short;
using   uint        = unsigned int;
using   ulong       = unsigned long;
using   llong       = long long;
using   ullong      = unsigned long long;
using   nullptr_t   = decltype(nullptr);

template<class T>
constexpr T* declptr() {
    return static_cast<T*>(nullptr);
}

template<class T>
constexpr const T& declval() {
    return *static_cast<T*>(nullptr);
}

template<class T>constexpr T* $declptr = nullptr;
template<class T>constexpr T& $declval = *static_cast<T*>(nullptr);
#pragma endregion

#pragma region type modifiers
template<class T> struct _Tconst            { using U = const T; };
template<class T> struct _Tconst<const T>   { using U = const T; };

template<class T> struct _Tmutable          { using U = T;       };
template<class T> struct _Tmutable<const T> { using U = T;       };

template<class T> struct _Treference        { using U = T&;      };
template<class T> struct _Treference<T&>    { using U = T&;      };

template<class T> struct _Tvalue            { using U = T;       };
template<class T> struct _Tvalue<T&>        { using U = T;       };

template<class T> using Tconst      = typename _Tconst<T>::U;
template<class T> using Tmutable    = typename _Tmutable<T>::U;
template<class T> using Treference  = typename _Treference<T>::U;
template<class T> using Tvalue      = typename _Tvalue<T>::U;

template<bool X, class T, class U>
struct _Tcond;

template<class A, class B> struct _Tcond<true,  A, B> { using U = A; };
template<class A, class B> struct _Tcond<false, A, B> { using U = B; };

template<bool  X, class A, class B=void>
using Tcond = typename _Tcond<X, A, B>::U;
#pragma endregion

#pragma region immutable
template<class T, T ...v>
struct Immutable
{
    constexpr static u32 $size = u32(sizeof...(v));
};

template<class T, T v>
struct Immutable<T, v>
{
    constexpr static u32 $size  = 1;
    constexpr static T   $value = v;
};

template<u32  ...V> using U32  = Immutable<u32, V...>;
template<i32  ...V> using I32  = Immutable<i32, V...>;
template<bool ...V> using Bool = Immutable<bool, V...>;

template<u32 N, u32 ...V> struct _Seq;
template<u32 N, u32 ...V> struct _Seq { using U = typename _Seq<N - 1, N - 1, V...>::U; };
template<       u32 ...V> struct _Seq<0, V...> { using U = U32<V...>; };

template<u32 N > using     Seq = typename _Seq<N>::U;
template<u32 N > constexpr Seq<N> $seq = {};

template<u32 K, class  I, bool ...V> struct _Index;
template<u32 K, u32 ...I           > struct _Index<K, U32<I...>             > { using U = U32<I...>; };
template<u32 K, u32 ...I, bool ...V> struct _Index<K, U32<I...>, true,  V...> { using U = typename _Index<K + 1, U32<I..., K>, V...>::U; };
template<u32 K, u32 ...I, bool ...V> struct _Index<K, U32<I...>, false, V...> { using U = typename _Index<K + 1, U32<I...   >, V...>::U; };

template<bool ...V> using       Index = typename _Index<0, U32<>, V...>::U;
template<bool ...V> constexpr   Index<V...> $index = {};

template<u32 I> struct Version;
template<u32 I> struct Version : public Version<I - 1> {};
template<>      struct Version<0> {};

namespace
{

template<i32  ...I> constexpr auto $i32 = Immutable<i32, I...>{};
template<u32  ...I> constexpr auto $u32 = Immutable<u32, I...>{};
template<bool ...I> constexpr auto $bool = Immutable<bool, I...>{};

constexpr auto $0 = $i32< 0>;
constexpr auto $1 = $i32< 1>;
constexpr auto $2 = $i32< 2>;
constexpr auto $3 = $i32< 3>;
constexpr auto $4 = $i32< 4>;
constexpr auto $5 = $i32< 5>;
constexpr auto $6 = $i32< 6>;
constexpr auto $7 = $i32< 7>;
constexpr auto $8 = $i32< 8>;
constexpr auto $9 = $i32< 9>;
constexpr auto $10 = $i32<10>;
constexpr auto $11 = $i32<11>;
constexpr auto $12 = $i32<12>;
constexpr auto $13 = $i32<13>;
constexpr auto $14 = $i32<14>;
constexpr auto $15 = $i32<15>;
constexpr auto $16 = $i32<16>;
constexpr auto $17 = $i32<17>;
constexpr auto $18 = $i32<18>;
constexpr auto $19 = $i32<19>;
}

#pragma endregion

#pragma region type cast
template<class T, class U>
static T& raw_cast(U&& u) noexcept {
    return *reinterpret_cast<T*>(&u);
}

template<class T> constexpr T&& fwd(Tvalue<T>&  t) noexcept { return (static_cast<T&&>(t)); }
template<class T> constexpr T&& fwd(Tvalue<T>&& t) noexcept { return (static_cast<T&&>(t)); }

template<class T> constexpr T&& move(T&  val) noexcept { return static_cast<T&&>(val); }
template<class T> constexpr T&& move(T&& val) noexcept { return static_cast<T&&>(val); }
#pragma endregion

#pragma region swap
#ifdef _UTILITY_
using std::swap;
#else
template<class T>
__forceinline void swap(T& a, T& b) noexcept {
    T c(static_cast<T&&>(a));
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(c);
}

template<class T, u32 N>
__forceinline void swap(T(&a)[N], T(&b)[N]) noexcept {
    for (u32 i = 0; i < N; ++i) {
        swap(a[i], b[i]);
    }
}
#endif
#pragma endregion

#pragma region vec types
template<class T, u32 ...Ns> struct Vec;
template<class T, u32 ...Ns> struct IVec;

template<class T, u32 ...Ns> using array = Vec<T, Ns...>;

template<class T, u32 N>
struct Vec<T, N>
{
    typedef       T(array_t)[N];
    typedef const T(const_array_t)[N];

    static constexpr auto $rank = 1u;
    static constexpr auto $count = N;
    T data_[$count];

    __forceinline constexpr Vec() noexcept
        : data_{ T() }
    {}

    template<class ...U>
    __forceinline constexpr Vec(const T &t, const U& ...u) noexcept
        : data_{ t, u... }
    {
        static_assert(1 + sizeof...(U) == $count, "invalid Vec elements count");
    }

    __forceinline constexpr Vec(const T(&arr)[N]) noexcept
        : Vec{ arr, Seq<N>{} } 
    {}

    /* property */
    __forceinline static constexpr u32 rank() {
        return $rank;
    }

    /* property */
    __forceinline static constexpr u32 count() noexcept {
        return $count;
    }

    __forceinline array_t&       data()       noexcept { return data_; }
    __forceinline const array_t& data() const noexcept { return data_; }

    __forceinline operator array_t&       ()       noexcept { return data(); }
    __forceinline operator const_array_t& () const noexcept { return data(); }

    /* index */
    template<class I> __forceinline T&       operator() (I idx)       noexcept { return data_[idx]; }
    template<class I> __forceinline const T& operator() (I idx) const noexcept { return data_[idx]; }
    template<class I> __forceinline T&       operator[] (I idx)       noexcept { return data_[idx]; }
    template<class I> __forceinline const T& operator[] (I idx) const noexcept { return data_[idx]; }

    bool operator==(const Vec& v) const {
        for (u32 i = 0; i < $rank; ++i) {
            if (data_[i] != v.data_[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Vec& v) const {
        return !(*this == v);
    }

protected:
    template<class U, u32 ...M>
    friend struct Vec;

    template<u32 ...I>
    __forceinline constexpr Vec(const T(&v)[N], U32<I...>) noexcept
        : data_{ v[I]...}
    {}

    template<u32 NX, u32 NY, u32 ...IX, u32 ...IY>
    __forceinline constexpr Vec(const T(&x)[NX], U32<IX...>, const T(&y)[NY], U32<IY...>) noexcept
        : data_{ x[IX]..., y[IY]... } 
    {}
};

/* --- vec utils --- */
template<class T, u32 N>
constexpr Vec<T, N> toVec(T(&v)[N]) {
    return v;
}



/* --- vec alias --- */
using i8x1  = Vec<i8,  1>; using u8x1  = Vec<u8, 1>;
using i16x1 = Vec<i16, 1>; using u16x1 = Vec<u16, 1>;
using i32x1 = Vec<i32, 1>; using u32x1 = Vec<u32, 1>;
using i64x1 = Vec<i64, 1>; using u64x1 = Vec<u64, 1>;
using f32x1 = Vec<f32, 1>; using f64x1 = Vec<f64, 1>;

using i8x2  = Vec<i8,  2>; using u8x2  = Vec<u8, 2>;
using i16x2 = Vec<i16, 2>; using u16x2 = Vec<u16, 2>;
using i32x2 = Vec<i32, 2>; using u32x2 = Vec<u32, 2>;
using i64x2 = Vec<i64, 2>; using u64x2 = Vec<u64, 2>;
using f32x2 = Vec<f32, 2>; using f64x2 = Vec<f64, 2>;

using i8x3  = Vec<i8,  3>; using u8x3  = Vec<u8, 3>;
using i16x3 = Vec<i16, 3>; using u16x3 = Vec<u16, 3>;
using i32x3 = Vec<i32, 3>; using u32x3 = Vec<u32, 3>;
using i64x3 = Vec<i64, 3>; using u64x3 = Vec<u64, 3>;
using f32x3 = Vec<f32, 3>; using f64x3 = Vec<f64, 3>;

using i8x4  = Vec<i8,  4>; using u8x4  = Vec<u8, 4>;
using i16x4 = Vec<i16, 4>; using u16x4 = Vec<u16, 4>;
using i32x4 = Vec<i32, 4>; using u32x4 = Vec<u32, 4>;
using i64x4 = Vec<i64, 4>; using u64x4 = Vec<u64, 4>;
using f32x4 = Vec<f32, 4>; using f64x4 = Vec<f64, 4>;

/* get elements count of this array */
template<class T, u32 N>
__forceinline constexpr u32 count(const T(&array)[N]) {
    (void)array;
    return N;
}

#pragma endregion

#pragma region iterator
template<class T>
struct PtrIterator
{
    PtrIterator(T* ptr, u32 step=1)
        : ptr_(ptr), step_(step)
    {}

    T& operator*() {
        return *ptr_;
    }

    const T& operator*() const {
        return *ptr_;
    }

    auto& operator++() {
        ptr_ += step_;
        return *this;
    }

    auto operator++(int) {
        auto tmp = *this;
        ptr_ += step_;
        return tmp;
    }

    friend constexpr bool operator==(const PtrIterator& lhs, const PtrIterator& rhs) noexcept {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend constexpr bool operator!=(const PtrIterator& lhs, const PtrIterator& rhs) noexcept {
        return lhs.ptr_ != rhs.ptr_;
    }

protected:
    T*  ptr_;
    u32 step_;
};

template<class T>
__forceinline PtrIterator<T> mkPtrIterator(T* ptr, u32 step) {
    return { ptr, step };
}

#pragma endregion

#pragma region noassign
class INoassignable
{
protected:
    INoassignable() = default;
    virtual ~INoassignable() = default;

    //! move-assign is default
    INoassignable& operator=(INoassignable&&) = default;

private:
    //! copy-assign is disabled
    INoassignable& operator=(const INoassignable&) = delete;
};
#pragma endregion

#pragma region no-assignable
class INocopyable : public INoassignable
{
protected:
    INocopyable()  = default;
    ~INocopyable() = default;

    //! move-constructor is default
    INocopyable(INocopyable&&) = default;

    //! copy-constructor is disabled
    INocopyable(const INocopyable&) = delete;
};
#pragma endregion

#pragma region string
template<class Tchar, u32 Isize>
class TString;

using String = TString<char, 0>;

template<u32 N = 32> using U8String  = TString<char,     N>;
template<u32 N = 32> using U16String = TString<char16_t, N>;
template<u32 N = 32> using U32String = TString<char32_t, N>;

#pragma endregion

#pragma region exception

class IException
{
protected:
    IException() {
    }

public:
    virtual ~IException() {}

    virtual void format(String& buf) const
    {}
};

NMS_API void dump(const IException& e);
#pragma endregion

#pragma region static init
template<class Tret, Tret(*func)(), class Tid>
struct StaticRunner
{
    StaticRunner() {
        (void)value;
    }

    static Tret value;
};

template<class Tret, Tret(*func)(), class Tid>
Tret StaticRunner<Tret, func, Tid>::value = (*func)();

template<u32(*func)(), class Tid>
u32 static_run() {
    StaticRunner<u32, func, Tid> static_runner;
    return static_runner.value;
}

#pragma endregion

}
