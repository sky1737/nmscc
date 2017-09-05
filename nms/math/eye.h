#pragma once

#include <nms/core.h>

namespace nms::math
{

/* Eye */
template<class T>
struct Eye
{
public:
    using Tview = Eye;
    constexpr static const auto $rank = 0;

    constexpr Eye() = default;

    template<class I>
    u32 size(I /*idx*/) const noexcept {
        return 0;
    }

    template<class ...I>
    T operator()(I ...i) const noexcept {
        static_assert($all_is<$int, I...>,  "unexpect type, shold be int");
        static constexpr auto N = u32(sizeof...(I));

        const T idx[] = { T(i)... };

        auto v = idx[0];
        for (u32 k = 1; k < N; ++k) {
            if (idx[k] != v) {
                return T(0);
            }
        }
        return T(1);
    }
};

template<class T>
constexpr auto eye() {
    return Eye<T>{};
}

}
