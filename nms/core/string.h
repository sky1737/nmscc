#pragma once

#include <nms/core/base.h>
#include <nms/core/view.h>
#include <nms/core/list.h>

namespace nms
{

NMS_API u32     strlen(const char* s);
NMS_API StrView cstr  (const char* s);

/* TString */
template<class Char, u32 Size=32>
class TString final
    : public List<Char, Size>
{
    using Tchar = Char;
    using base = nms::List<Char, Size>;

    using base::Tsize;
    using base::Tdata;

public:
#pragma region constructor
    /*! construct a empty TString */
    constexpr TString() noexcept {
    }

    /* destructor */
    ~TString() = default;

    TString(const Tchar buff[], Tsize count) {
        base::appends(buff, count);
    }

    /* constructor: redirect to List */
    template<Tsize N>
    __forceinline TString(const Tchar(&s)[N])
        : TString{ s, N - 1 } {
    }

    TString(const StrView& rhs)
        : TString(rhs.data(), rhs.count()) {
    }

    TString(const TString& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}

#pragma endregion

    /*! resize the TString */
    TString& resize(Tsize n) {
        base::reserve(n);
        base::size_ = n;
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
