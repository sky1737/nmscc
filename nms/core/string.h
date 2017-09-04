#pragma once

#include <nms/core/base.h>
#include <nms/core/view.h>
#include <nms/core/list.h>

namespace nms
{

template<class T, u32 S=0>
class TString final;

/* TString */
template<class T>
class TString<T, 0>
    : public List<T, 0>
{
    using Tchar = T;
    using base  = nms::List<Tchar>;
    using Tsize = base::Tsize;
    using Tdata = base::Tdata;

public:
#pragma region constructor
    constexpr TString() noexcept = default;

    ~TString() = default;

    TString(const Tchar buff[], Tsize count) {
        base::appends(buff, count);
    }

    /* constructor: redirect to List */
    template<Tsize N>
    TString(const Tchar(&s)[N])
        : TString{ s, N - 1 } {
    }

    TString(const StrView& rhs)
        : TString(rhs.data(), rhs.count()) {
    }

    TString(const TString& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}

#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;
    using base::capicity;
#pragma endregion

#pragma region method        
    /*! reserve the storge */
    using base::reserve;

    /*! resize the TString */
    TString& resize(Tsize n) {
        reserve(n);
        size_ = n;
        return *this;
    }

    /*! returns a cstring (null terminal) */
    const Tchar* cstr() const {
        if (base::count() == 0) {
            return nullptr;
        }

        auto& self = const_cast<TString&>(*this);
        if (base::at(count()-1) != '\0') {
            const auto oldlen = self.count();
            const auto newlen = oldlen+1;
            self.reserve(newlen);
            self.data()[oldlen] = '\0';
        }
        return base::data();
    }
#pragma endregion 

    TString& operator+=(View<Tchar> s) {
        base::appends(s.data(), s.count());
        return *this;
    }

    TString& operator+=(View<const Tchar> s) {
        base::appends(s.data(), s.count());
        return *this;
    }

    TString& operator+=(Tchar c) {
        base::append(c);
        return *this;
    }

    /*! find the first index of c */
    Tsize find(Tchar c) const {
        const auto n = base::count();
        for (Tsize i = 0; i < n; ++i) {
            if (base::at(i) == c) return i;
        }
        return n;
    }

protected:
    using base::size_;
    using base::capicity_;
    using base::data_;
};













using U8String  = TString<char>;
using U16String = TString<char16_t>;
using U32String = TString<char32_t>;
using String    = TString<char>;

template<class Tchar=char>
TString<Tchar>& tlsString() {
    static thread_local TString<Tchar> buf ;

    static thread_local auto _init = [&] {
        buf.reserve(32768);
        return 0;
    }();
    (void)_init;

    return buf;
}

/**
 * concatenates two TStrings
 */
inline U8String operator+(StrView a, StrView b) {
    U8String c;
    c.reserve(a.count() + b.count() +1);
    c += a;
    c += b;
    return c;
}

/* split a TString into pieces */
NMS_API List<StrView> split(StrView str, StrView delimiters);

}
