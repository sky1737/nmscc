#pragma once

#include <nms/core.h>

namespace nms::math
{

#pragma region view
template<class T>
auto _view_cast(const T& t, Version<0> ) -> Scalar<T> {
    return t;
}

template<class T>
auto _view_cast(const T& t, Version<1> ) -> typename T::Tview {
    return t;
}

template<class T>
auto view_cast(const T&t) {
    return _view_cast(t, Version<1>{});
}


template<class X, class Y, class= typename X::Tview, class = typename Y::Tview>
auto _view_test_xy(Version<2>)  {
    return 0;
}

template<class X, class Y, class = typename X::Tview, class = $when<($is<$number, Y> || $is<bool, Y>)> >
auto _view_test_xy(Version<1>)  {
    return 0;
}

template<class X, class Y, class = $when_is<$number, X>, class = typename Y::Tview>
auto _view_test_xy(Version<0>)  {
    return 0;
}

template<class X, class Y>
auto view_test_xy() -> decltype(_view_test_xy<X, Y>(Version<2>{})) {
    return 0;
}
#pragma endregion

#pragma region Parallel
template<class F, class ...T>
struct Parallel;

template<class F, class T>
struct Parallel<F, T>
{
    using Tview = Parallel;

    constexpr static const auto $rank = T::$rank;

    Parallel(const T& t)
        : t_(t) {}

    template<class I>
    auto size(I idx) const noexcept {
        return t_.size(idx);
    }

    template<class ...I>
    auto operator()(I ...idx) const noexcept {
        return F::run(t_(idx...));
    }

protected:
    T   t_;
};

template<class F, class X, class Y>
struct Parallel<F, X, Y>
{
    using Tview = Parallel;

    constexpr static const auto $rank = X::$rank | Y::$rank;

    Parallel(const X& x, const Y& y)
        : x_(x), y_(y) {}

    template<class I>
    auto size(I idx) const noexcept {
        const auto sx = x_.size(idx);
        const auto sy = y_.size(idx);
        return X::$rank == 0 ? sy : Y::$rank == 0 ? sx : nms::min(sx, sy);
    }

    template<class ...I>
    auto operator()(I ...idx) const noexcept {
        return F::run(x_(idx...), y_(idx...));
    }

protected:
    X   x_;
    Y   y_;
};

/* make Parallel<F(x)> */
template<class F, class X>
auto mkParallel(const X& x) {
    using Vx = decltype(view_cast(x));
    return Parallel<F, Vx>{ x };
}

/* make Parallel<F(A,B)> */
template<class F, class X, class Y>
auto mkParallel(const X& x, const Y& y) {
    using Vx = decltype(view_cast(x));
    using Vy = decltype(view_cast(y));
    return Parallel<F, Vx, Vy>{ x, y };
}

#pragma endregion

#pragma region Reduce
template<class F, class ...T>
struct Reduce;

template<class F, class X>
struct Reduce<F, X>
{
    using Tview = Reduce;
    constexpr static const auto $rank = X::$rank - 1;

    Reduce(const X& x) noexcept
        : x_(x)
    {}

    template<class I>
    auto size(I idx) const noexcept {
        return x_.size(idx + 1);
    }

    template<class ...I>
    auto operator()(I ...idx) const {
        const auto  n = x_.size(0);

        if (n < 2) {
            return x_(0, idx...);
        }

        auto ret = F::run(x_(0, idx...), x_(1, idx...));
        for (u32 i = 2; i < n; ++i) {
            ret = F::run(ret, x_(i, idx...));
        }

        return ret;
    }

private:
    X   x_;
};

template<class F, class X>
auto mkReduce(const X& x) -> Reduce<F, decltype(view_cast(x)) > {
    return { view_cast(x) };
}

#pragma endregion

#pragma region fureach-executor
struct Texec
{
    template<class Tfunc, class Tret, class ...Targs>
    void foreach(Tfunc fun, Tret& ret, const Targs& ...args) {
        _foreach(U32<Tret::$rank>{}, fun, ret, args...);
    }

protected:
    template<class Tfunc, class Tret, class Targ>
    void _foreach(U32<1>, Tfunc func, Tret& ret, const Targ& arg) {
        const auto size = ret.size();

        for (u32 i0 = 0; i0 < size[0]; ++i0) {
            func(ret(i0), arg(i0));
        }
    }

    template<class Tfunc, class Tret, class Targ>
    void _foreach(U32<2>, Tfunc func, Tret& ret, const Targ& arg) {
        const auto size = ret.size();

        for (u32 i1 = 0; i1 < size[1]; ++i1) {
            for (u32 i0 = 0; i0 < size[0]; ++i0) {
                func(ret(i0, i1), arg(i0, i1));
            }
        }
    }

    template<class Tfunc, class Tret, class Targ>
    void _foreach(U32<3>, Tfunc func, Tret& ret, const Targ& arg) {
        const auto size = ret.size();

        for (u32 i2 = 0; i2 < size[2]; ++i2) {
            for (u32 i1 = 0; i1 < size[1]; ++i1) {
                for (u32 i0 = 0; i0 < size[0]; ++i0) {
                    func(ret(i0, i1, i2), arg(i0, i1, i2));
                }
            }
        }
    }

    template<class Tfunc, class Tret, class Targ>
    void _foreach(U32<4>, Tfunc fun, Tret& ret, const Targ& arg) {
        const auto size = ret.size();

        for (u32 i3 = 0; i3 < size[3]; ++i3) {
            for (u32 i2 = 0; i2 < size[2]; ++i2) {
                for (u32 i1 = 0; i1 < size[1]; ++i1) {
                    for (u32 i0 = 0; i0 < size[0]; ++i0) {
                        fun(ret(i0, i1, i2, i3), arg(i0, i1, i2, i3));
                    }
                }
            }
        }
    }
};

/* combine executor */
inline Texec operator||(const Texec&, const Texec&) {
    return {};
}

template<class T>
auto _mk_exec(const T&, Version<0>) -> Texec {
    return {};
}

template<class T>
auto _mk_exec(const T&, Version<1>) -> typename T::Texec {
    return {};
}

template<class T>
auto get_exec(const T& t) {
    return _mk_exec(t, Version<1>{});
}

template<class T, class U, class ...R>
auto get_exec(const T& t, const U& u, const R& ...r) {
    auto et = get_exec(t);
    auto eu = get_exec(u, r...);
    return et || eu;
}

template<class T>
bool check_size(const T& t) {
    return true;
}

template<class T, class U, class ...R>
bool check_size(const T& t, const U& u, const R& ...r) {
    static_assert(T::$rank == U::$rank || U::$rank == 0, "nms.math._check_size: $rank not match");

    for (u32 i = 0; i < T::$rank; ++i) {
        if (u.size(i) != 0 && u.size(i) > t.size(i)) {
            return false;
        }
    }
    return check_size(t, r...);
}

template<class Tfunc, class Tret, class ...Targs>
bool foreach(Tfunc func, Tret& ret, const Targs& ...args) {
    if (!check_size(view_cast(ret), view_cast(args)...)) {
        return false;
    }

    auto executor = get_exec(ret, args...);
    (void)executor;

    typename Tret::Tview lret(ret);
    executor.foreach(func, lret, view_cast(args)...);
    return true;
}

#pragma endregion

#pragma region functions

#define NMS_IVIEW_FOREACH(op, type)                     \
    template<class T, class = typename T::Tview>        \
    constexpr auto operator op(const T& t) noexcept {   \
            return math::mkParallel<type>(t);           \
    }
NMS_IVIEW_FOREACH(+, Pos)
NMS_IVIEW_FOREACH(-, Neg)
#undef NMS_IVIEW_FOREACH

#define NMS_IVIEW_FOREACH(op, type)                                     \
    template<class X, class Y, class = decltype(view_test_xy<X, Y>())>  \
    constexpr auto operator op(const X& x, const Y& y) noexcept {       \
        return math::mkParallel<type>(x, y);                            \
    }

NMS_IVIEW_FOREACH(+ , Add)
NMS_IVIEW_FOREACH(- , Sub)
NMS_IVIEW_FOREACH(* , Mul)
NMS_IVIEW_FOREACH(/ , Div)
NMS_IVIEW_FOREACH(^ , Pow)

NMS_IVIEW_FOREACH(== ,  Eq)
NMS_IVIEW_FOREACH(!= , Neq)
NMS_IVIEW_FOREACH(<,   Lt)
NMS_IVIEW_FOREACH(>,   Gt)
NMS_IVIEW_FOREACH(<=,  Le)
NMS_IVIEW_FOREACH(>=,  Ge)
#undef NMS_IVIEW_FOREACH

template<class X, class Y, class=typename Y::Tview>
Y& operator<<=(Y& y, const X& x) {
    foreach(Ass2{}, y, x);
    return y;
}

template<class X, class Y, class=typename Y::Tview>
Y& operator>>=(const X& x, Y& y) {
    foreach(Ass2{}, y, x);
    return y;
}

#define NMS_IVIEW_FOREACH(op, type)                     \
template<class X, class Y, class=typename Y::Tview >    \
Y& operator op(Y& y, const X& x) {                      \
    foreach(type{}, y, x);                              \
    return y;                                           \
}

NMS_IVIEW_FOREACH(+=, Add2)
NMS_IVIEW_FOREACH(-=, Sub2)
NMS_IVIEW_FOREACH(*=, Mul2)
NMS_IVIEW_FOREACH(/=, Div2)
#undef NMS_IVIEW_FOREACH

#define NMS_IVIEW_FOREACH(func, type)       \
template<class T>                           \
constexpr auto func(const T& t) noexcept {  \
    return math::mkParallel<type>(t);       \
}
NMS_IVIEW_FOREACH(vabs,    Abs)
NMS_IVIEW_FOREACH(vsqrt,   Sqrt)
NMS_IVIEW_FOREACH(vpow2,   Pow2)
NMS_IVIEW_FOREACH(vexp,    Exp)
NMS_IVIEW_FOREACH(vln,     Ln)
NMS_IVIEW_FOREACH(vlog10,  Log10)

NMS_IVIEW_FOREACH(vsin,    Sin)
NMS_IVIEW_FOREACH(vcos,    Cos)
NMS_IVIEW_FOREACH(vtan,    Tan)

NMS_IVIEW_FOREACH(vasin,   Atan)
NMS_IVIEW_FOREACH(vacos,   Acos)
NMS_IVIEW_FOREACH(vatan,   Atan)
#undef NMS_IVIEW_FOREACH

#define NMS_IVIEW_REDUCE(func, type)        \
template<class T>                           \
constexpr auto func(const T& t) noexcept {  \
    return math::mkReduce<type>(t);         \
}
NMS_IVIEW_REDUCE(vsum,      Add)
NMS_IVIEW_REDUCE(vmax,      Max)
NMS_IVIEW_REDUCE(vmin,      Min)
#undef  NMS_IVIEW_REDUCE

}
