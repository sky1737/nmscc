#pragma once

#include <nms/core/trait.h>

namespace nms
{

template<class T, class=$when_is<$number, T>>
constexpr auto (min)(const T& a, const T& b) {
    return a < b ? a : b;
}

template<class T, class=$when_is<$number, T>>
constexpr auto (max)(const T& a, const T& b) {
    return a < b ? a : b;
}

constexpr auto any(bool a) {
    return a;
}

constexpr auto any(bool a, bool b) {
    return a || b;
}

constexpr auto any(bool a, bool b, bool c) {
    return a || b || c;
}

constexpr auto any(bool a, bool b, bool c, bool d) {
    return a || b || c || d;
}

constexpr auto any(bool a, bool b, bool c, bool d, bool e) {
    return a || b || c || d || e;
}

constexpr auto all(bool a) {
    return a;
}

constexpr auto all(bool a, bool b) {
    return a && b;
}

constexpr auto all(bool a, bool b, bool c) {
    return a && b && c;
}

constexpr auto all(bool a, bool b, bool c, bool d) {
    return a && b && c && d;
}

constexpr auto all(bool a, bool b, bool c, bool d, bool e) {
    return a && b && c && d && e;
}

template<class T, class=$when_is<$number, T>>
constexpr auto sum(const T& a) {
    return a;
}

template<class T, class=$when_is<$number, T>>
constexpr auto sum(const T& a, const T& b) {
    return a + b;
}

template<class T, class=$when_is<$number, T>>
constexpr auto sum(const T& a, const T& b, const T& c) {
    return a + b + c;
}

template<class T, class=$when_is<$number, T>>
constexpr auto sum(const T& a, const T& b, const T& c, const T& d) {
    return a + b + c + d;
}

template<class T, class=$when_is<$number, T>>
constexpr auto sum(const T& a, const T& b, const T& c, const T& d, const T& e) {
    return a + b + c + d + e;
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod() {
    return T(1);
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod(const T& a) {
    return a;
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod(const T& a, const T& b) {
    return a*b;
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod(const T& a, const T& b, const T& c) {
    return a*b*c;
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod(const T& a, const T& b, const T& c, const T& d) {
    return a*b*c*d;
}

template<class T, class=$when_is<$number, T>>
constexpr auto prod(const T& a, const T& b, const T& c, const T& d, const T& e) {
    return a*b*c*d*e;
}

template<class V>
constexpr auto iprod(U32<>, const V& v) {
    return decltype(v[0])(1);
}

template<u32 ...I, class V>
constexpr auto iprod(U32<I...>, const V& v) {
    return prod(v[I]...);
}

}
