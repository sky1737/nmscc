#pragma once

#include <nms/core/base.h>
#include <nms/core/view.h>
#include <nms/core/list.h>

namespace nms
{

NMS_API u32 strlen(const char* s);

template<class T, u32 N=0>
class TString;

/* TString */
template<class T>
class TString<T, 0>: public List<T, 0>
{
public:
    using Tchar = T;
    using base  = List<Tchar>;
    using Tsize = typename base::Tsize;
    using Tdata = typename base::Tdata;
    using Tview = View<const T>;

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

    TString(const Tview& rhs)
        : TString(rhs.data(), rhs.count()) {
    }

    TString(const TString& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}

    TString& operator=(const Tview& s) {
        base::size_ = 0;
        *this += s;
        return *this;
    }

    template<u32 SN>
    TString& operator=(const Tchar(&s)[SN]) {
        base::size_ = 0;
        *this += Tview{ s };
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

    TString& operator+=(const Tview& s) {
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
    using Tview = View<const T>;

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

    TString(const Tview& rhs)
        : TString(rhs.data(), rhs.count())
    {}

    TString(TString&& rhs) noexcept
        : TString{}
    {
        size_ = rhs.size_;
        if (rhs.data_ == rhs.buff_) {
            data_ = buff_;
            for (u32 i = 0; i < $capicity; ++i) {
                buff_[i] = rhs.buff_[i];
            }
        }
        else {
            data_ = rhs.data_;
        }


        rhs.data_ = nullptr;
    }

    TString(const TString& rhs)
        : TString{ rhs.data(), rhs.count() }
    {}


    TString& operator=(const Tview& s) {
        base::operator=(s);
        return *this;
    }

    template<u32 SN>
    TString& operator=(const Tchar(&s)[SN]) {
        base::operator=(s);
        return *this;
    }

#pragma endregion

protected:
    using base::size_;
    using base::capacity_;
    using base::data_;
    Tchar buff_[$capicity] = {};
};

/* split a TString into pieces */
NMS_API List<StrView> split(StrView str, StrView delimiters);

}
