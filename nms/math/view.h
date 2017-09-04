#pragma once

#include <nms/core.h>

namespace nms::math
{

using _view::IView;

// view_cast: -> Tmath
template<class X>
auto view_cast(const X& x, Version<1> = {} ) -> typename X::Tview {
    return x;
}

#pragma region scalar

template<class T>
struct Scalar : public IView<T, 0>
{
    using Tview = Scalar;

    Scalar(const T& t)
        : t_(t) {}

    template<class I>
    u32 size(I idx) const noexcept {
        return 1u;
    }

    template<class ...I>
    const T& operator()(I ...idx) const noexcept {
        return t_;
    }

protected:
    T   t_;
};

/* mk scalar */
template<class T>
Scalar<T> mkScalar(const T& t) {
    return { t };
}

/* to scalar */
template<class T, u32 N>
auto toScalar(const Vec<T, N>& t) {
    return Scalar< Vec<T, N> >{t};
}

/* to scalar */
template<class T, class = $when< $is<$number, T> || $is<bool, T> > >
auto toScalar(const T& t) {
    return Scalar<T>{t};
}

/* view_cast: -> Scalar */
template<class X>
auto view_cast(const X& x, Version<0> = {}) -> decltype(toScalar(x)) {
    return toScalar(x);
}
#pragma endregion

#pragma region Parallel
template<class F, class ...T>
struct Parallel;

template<class F, class T>
struct Parallel<F, T>
    : IView<decltype(F::run(declval<typename T::Tdata>())), T::$rank>
{
    using Tview = Parallel;

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
    : IView<decltype(F::run(declval<typename X::Tdata>(), declval<typename Y::Tdata>())), (X::$rank > Y::$rank ? X::$rank : Y::$rank) >
{
    using Tview = Parallel;

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
    : IView<decltype(F::run(declval<typename X::Tdata>(), declval<typename X::Tdata>())), X::$rank - 1>
{
    using Tview = Reduce;

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
    if (!check_size(ret, args...)) {
        return false;
    }

    auto executor = get_exec(ret, args...);
    (void)executor;

    typename Tret::Tview lret(ret);
    executor.foreach(func, lret, view_cast(args)...);
    return true;
}

#pragma endregion

namespace _view
{

#pragma region view_cast

/* view_cast */
template<class X>
auto view_cast(const X& x) -> decltype(_view_cast(x, Version<1>{})) {
    return _view_cast(x, Version<1>{});
}

#pragma endregion

#pragma region view_test
template<class X, class Y>
auto _view_test(const X& x, const Y&y, Version<2>) ->decltype(X::Tview(x), Y::Tview(y), 0) {
    return 0;
}

template<class X, class Y>
auto _view_test(const X& x, const Y&y, Version<1>) ->decltype(X::Tview(x), toScalar(y), 0) {
    return 0;
}

template<class X, class Y>
auto _view_test(const X& x, const Y&y, Version<0>) ->decltype(toScalar(x), Y::Tview(y), 0) {
    return 0;
}

template<class X, class Y>
auto view_test(const X& x, const Y& y) -> decltype(_view_test(x, y, Version<2>{}), 0) {
    return 0;
}

#pragma endregion

#pragma region functions

#define NMS_IVIEW_PARALLEL(op, type) template<class T> constexpr auto operator op(const T& t) noexcept { return math::mkParallel<type>(t); }
NMS_IVIEW_PARALLEL(+, Pos)
NMS_IVIEW_PARALLEL(-, Neg)
#undef NMS_IVIEW_PARALLEL

#define NMS_IVIEW_PARALLEL(op, type) template<class X, class Y> constexpr auto operator op(const X& x, const Y& y) noexcept { return math::mkParallel<type>(x, y); }
NMS_IVIEW_PARALLEL(+ , Add)
NMS_IVIEW_PARALLEL(- , Sub)
NMS_IVIEW_PARALLEL(* , Mul)
NMS_IVIEW_PARALLEL(/ , Div)
NMS_IVIEW_PARALLEL(^ , Pow)

NMS_IVIEW_PARALLEL(== ,  Eq)
NMS_IVIEW_PARALLEL(!= , Neq)
NMS_IVIEW_PARALLEL(<,   Lt)
NMS_IVIEW_PARALLEL(>,   Gt)
NMS_IVIEW_PARALLEL(<=,  Le)
NMS_IVIEW_PARALLEL(>=,  Ge)
#undef NMS_IVIEW_PARALLEL


#define NMS_IVIEW_PARALLEL(func, type) template<class T> constexpr auto func(const T& t) noexcept { return math::mkParallel<type>(t); }
NMS_IVIEW_PARALLEL(abs,    Abs)
NMS_IVIEW_PARALLEL(sqrt,   Sqrt)
NMS_IVIEW_PARALLEL(pow2,   Pow2)
NMS_IVIEW_PARALLEL(exp,    exp)
NMS_IVIEW_PARALLEL(ln,     Ln)
NMS_IVIEW_PARALLEL(log10, Log10)

NMS_IVIEW_PARALLEL(sin,    Tan)
NMS_IVIEW_PARALLEL(cos,    Cos)
NMS_IVIEW_PARALLEL(tan,    Tan)

NMS_IVIEW_PARALLEL(asin,   Atan)
NMS_IVIEW_PARALLEL(acos,   Acos)
NMS_IVIEW_PARALLEL(atan,   Atan)
#undef NMS_IVIEW_PARALLEL


#define NMS_IVIEW_REDUCE(func, type) template<class T> constexpr auto func(const T& t) noexcept { return math::mkReduce<type>(t); }
NMS_IVIEW_REDUCE(sum, Add)
NMS_IVIEW_REDUCE(max, Max)
NMS_IVIEW_REDUCE(min, Min)
#undef  NMS_IVIEW_REDUCE

#pragma endregion

#pragma region foreach-operator
template<class X, class Y>
Y& operator<<=(Y& y, const X& x) {
    foreach(Ass2{}, y, x);
    return y;
}

template<class X, class Y>
Y& operator>>=(const X& x, Y& y) {
    foreach(Ass2{}, y, x);
    return y;
}

template<class X, class Y>
Y& operator+=(Y& y, const X& x) {
    foreach(Add2{}, y, x);
    return y;
}

template<class X, class Y>
Y& operator-=(Y& y, const X& x) {
    foreach(Sub2{}, y, x);
    return y;
}

template<class X, class Y>
Y& operator*=(Y& y, const X& x) {
    foreach(Mul2{}, y, x);
    return y;
}

template<class X, class Y>
Y& operator/=(Y& y, const X& x) {
    foreach(Div2{}, y, x);
    return y;
}
#pragma endregion

}

}
