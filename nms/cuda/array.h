#pragma once

#include <nms/core.h>
#include <nms/math.h>
#include <nms/cuda/runtime.h>
#include <nms/cuda/engine.h>

namespace nms::cuda
{

namespace device
{
template<class T, u32 N>
class Array
    : public View<T, N>
    , public INocopyable
{
public:
    using base  = View<T, N>;
    using Texec = cuda::Texec;
    using Tsize = typename base::Tsize;

    explicit Array(const Tsize(&len)[N])
        : base(nullptr, len) {
        auto dat = mnew<T>(base::count());
        base::data_ = dat;
        deleter_ = delegate<void()>([=] { mdel(dat); });
    }

    virtual ~Array() {
        deleter_();
        base::data_ = nullptr;
    }

    Array(Array&& rhs) noexcept
        : base(static_cast<base&&>(rhs)) {
        base::_dat = nullptr;
    }

    Array& operator=(Array&& rhs) noexcept {
        base::operator  = (rhs);
        deleter_        = move(rhs.deleter_);
        return *this;
    }

protected:
    Array(const Array& rhs)
        : Array(rhs.len()) {
        *this <<= rhs;
    }

private:
    delegate<void()>    deleter_;
};
}

namespace host
{

template<class T, u32 N>
class Array
    : public View<T, N>
    , public INocopyable
{
public:
    using base = View<T, N>;
    using Texec = cuda::Texec;
    using Tsize = typename base::Tsize;

    constexpr static const auto $rank = base::$rank;

    explicit Array(const Tsize(&size)[$rank])
        : base(nullptr, size) {
        auto dat    = hnew<T>(base::count());
        base::data_ = dat;
        deleter_ = [=] { hdel(dat); };
    }

    virtual ~Array() {
        deleter_();
        base::data_ = nullptr;
    }

    Array(Array&& rhs) noexcept
        : base(rhs) {
        rhs.data_ = nullptr;
    }

    Array& operator=(Array&& rhs) noexcept {
        if (this != &rhs) {
            Array tmp(move(rhs));
            nms::swap(static_cast<base&>(*this), static_cast<base&>(tmp));
            nms::swap(deleter_, tmp.deleter_);
        }
        return *this;
    }

protected:
    Array(const Array& rhs)
        : Array(rhs.size()) {
        *this <<= rhs;
    }

private:
    delegate<void()>    deleter_;
};

}

}
