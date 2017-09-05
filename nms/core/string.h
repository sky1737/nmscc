#pragma once

#include <nms/core/base.h>
#include <nms/core/view.h>
#include <nms/core/list.h>

namespace nms
{

NMS_API u32 strlen(const char* s);

inline StrView mkStrView(const char* s) {
    return {s, strlen(s)};
}

template<class T, u32 N=0>
class TString;

/* TString */
template<class T>
class TString<T, 0> : public List<T, 0>
{
public:
    using Tchar = T;
    using base  = List<Tchar>;
    using Tsize = typename base::Tsize;
    using Tdata = typename base::Tdata;

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

    TString(const View<const Tchar>& rhs)
        : TString(rhs.data(), rhs.count()) {
    }

    TString(TString&& rhs) noexcept
        : base(static_cast<base&&>(rhs)) {
    }

    TString(const TString& rhs) noexcept
        : TString(rhs.data(), rhs.count())
    { }

    TString& operator=(const View<const Tchar>& s) {
        base::size_ = 0;
        *this += s;
        return *this;
    }

    template<u32 SN>
    TString& operator=(const Tchar(&s)[SN]) {
        base::size_ = 0;
        *this += View<const Tchar>{ s };
        return *this;
    }

#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;
    using base::capacity;
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
        if (data_ == nullptr) {
            return nullptr;
        }

        if (data_[size_] != '\0') {
            auto& self = const_cast<TString&>(*this);
            self.reserve(size_+1);
            self.data_[size_] = '\0';
        }
        return data_;
    }
#pragma endregion 

    TString& operator+=(const View<const Tchar>& s) {
        base::appends(s.data(), s.count());
        return *this;
    }

    TString& operator+=(Tchar c) {
        base::append(c);
        return *this;
    }

    template<class U>
    TString operator+(U&& u) const {
        TString tmp(*this);
        tmp += fwd<U>(u);
        return tmp;
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
    using base::capacity_;
    using base::data_;
};

/* TString */
template<class T, u32 N>
class TString: public TString<T>
{
public:
    using Tchar = T;
    using base  = TString<T, 0>;
    using Tsize = typename base::Tsize;
    using Tdata = typename base::Tdata;

    static const auto $capicity = N;

public:
#pragma region constructor
    constexpr TString() noexcept {
        base::data_     = buff_;
        base::capacity_ = $capicity;
    }

    ~TString()
    {}

    TString(const Tchar buff[], Tsize count)
        : TString{}
    {
        base::appends(buff, count);
    }

    /* constructor: redirect to List */
    template<Tsize SN>
    TString(const Tchar(&s)[SN])
        : TString{ s, SN - 1 }
    {}

    TString(const View<const Tchar>& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}

    TString(TString&& rhs) noexcept
        : base(static_cast<base&&>(rhs))
    {
        if (data_ == rhs.buff_) {
            dat2buf();
        }
    }

    TString(const TString& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}


    TString& operator=(TString&& rhs) noexcept {
        if (this != &rhs) {
            base::oeprator = (static_cast<base&&>(rhs));
            if (data_ == rhs.buff_) {
                dat2buf();
            }
        }
        return *this;
    }
    TString& operator=(const View<const Tchar>& s) {
        base::operator=(s);
        return *this;
    }

    template<u32 SN>
    TString& operator=(const Tchar(&s)[SN]) {
        base::operator=(s);
        return *this;
    }

    template<class U>
    TString operator+(U&& u) const {
        TString tmp(*this);
        tmp += fwd<U>(u);
        return tmp;
    }

#pragma endregion

protected:
    using base::size_;
    using base::capacity_;
    using base::data_;
    Tchar buff_[$capicity] = {};

    void dat2buf() {
        auto buf = buff_;
        auto dat = data_;
        for (Tsize i = 0; i < size_; ++i) {
            new(&buf[i]) Tdata(static_cast<Tdata&&>(dat[i]));
        }
        data_ = buf;
    }
};

/* split a TString into pieces */
NMS_API List<StrView> split(StrView str, StrView delimiters);

}
