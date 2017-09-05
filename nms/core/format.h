#pragma once

#include <nms/core/type.h>
#include <nms/core/cpp.h>
#include <nms/core/view.h>
#include <nms/core/string.h>
#include <nms/core/exception.h>
#include <nms/util/stacktrace.h>

namespace nms
{

void format_switch(String& buf, const StrView& fmt, ...);

struct IFormatable
{

    template<class T>
    static void _format(String& buf, const T& obj) {
        auto name_len = 4u;
        _do_format(nullptr, obj, name_len);
        name_len = (name_len + 3) / 4 * 4;  // 4 spaces indent
        _do_format(&buf   , obj, name_len);
    }
private:
    template<class T>
    static void _do_format(String* buf, const T& obj, u32& name_len) {
        if (buf!=nullptr) *buf += "{\n";

#define call_do_format(n, ...)    _do_format(I32<n>{}, buf, obj, name_len);
        NMSCPP_LOOP_99(call_do_format)
#undef call_do_format

        if (buf!=nullptr) *buf += "}\n";
    }

    // format-do
    template<class T, i32 I>
     static auto _do_format(I32<I> idx, String* buf, const T& obj, u32& name_len) ->$when<(I < T::_$property_cnt)> {
        auto t = (obj)[idx];
        auto& name  = t.name;
        auto& value = t.value;
        if (buf != nullptr) {
            buf->appends(4, ' ');
            *buf += name;
            buf->appends(name_len - name.count(), ' ');
            *buf += StrView(": ");
            format_switch(*buf, {}, value);
            *buf += StrView("\n");
        }
        else {
            if (name_len < name.count()) {
                name_len = name.count();
            }
        }
        return;
    }

    // format-end
    template<class T, i32 I >
    static auto _do_format(I32<I>, String* , const T&, ...) -> $when<(I >= T::_$property_cnt)> {
        return;
    }
};

#pragma region format impl

NMS_API void formatImpl(String& buf, const StrView& fmt, i8       val);
NMS_API void formatImpl(String& buf, const StrView& fmt, u8       val);
NMS_API void formatImpl(String& buf, const StrView& fmt, i16      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, u16      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, i32      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, u32      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, i64      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, u64      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, f32      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, f64      val);
NMS_API void formatImpl(String& buf, const StrView& fmt, void*    val);
NMS_API void formatImpl(String& buf, const StrView& fmt, StrView  val);
NMS_API void formatImpl(String& buf, const StrView& fmt, bool     val);

NMS_API void formatImpl(String& buf, const StrView& fmt, const IException&  val);

inline  void formatImpl(String& buf, const StrView& fmt, const String& val) {
    formatImpl(buf, fmt, StrView(val));
}

template<u32 N>
void formatImpl(String& buf, const StrView& fmt, const char(&v)[N]) {
    formatImpl(buf, fmt, cstr(v));
}

inline void formatImpl(String& buf, const StrView& fmt, const char* str) {
    formatImpl(buf, fmt, StrView{ str, u32(strlen(str)) });
}


template<class T, u32 N>
void formatImpl(String& buf, const StrView& fmt, const Vec<T, N>& v) {
    buf += "[";
    for (u32 i = 0; i < N; ++i) {
        format_switch(buf, fmt, v[i]);
        if (i != N - 1) buf += ", ";
    }
    buf += "]";
}

template<class T, u32 N>
void formatImpl(String& buf, const StrView& fmt, const T(&v)[N]) {
    buf += "[";
    for (u32 i = 0; i < N; ++i) {
        format_switch(buf, fmt, v[i]);
        if (i != N - 1) buf += ", ";
    }
    buf += "]";
}

template<class T, u32 N>
void formatImpl(String& buf, const StrView& fmt, const List<T, N>& v) {
    const auto n = v.count();
    buf += "[";
    for (u32 i = 0; i < n; ++i) {
        format_switch(buf, fmt, v[i]);
        if (i != n - 1) buf += ", ";
    }
    buf += "]";
}


/*!
* format:
* type: view<T,1>
* fmt:  ?...?
*/
template<class T>
void formatImpl(String& buf, const StrView& fmt, const View<T, 1>& v) {
    static const StrView delimiter = ", ";
    buf.reserve(buf.count() + v.count() * 8);

    const auto nx = v.count();

    for (u32 x = 0; x < nx; ++x) {
        format_switch(buf, fmt, v(x));
        if (x != nx - 1) {
            buf += delimiter;
        }
    }
}


/* format: view<T,2> */
template<class T>
void formatImpl(String& buf, const StrView& fmt, const View<T, 2>& v) {
    buf.reserve(buf.count() + v.count() * 8);

    const auto cnt = v.size(1);

    buf += StrView{ "\n" };
    for (u32 i1 = 0; i1 < cnt; ++i1) {
        buf += StrView{ "    |" };
        formatImpl(buf, fmt, v.slice({ 0, -1 }, { i1 }));
        if (i1 + 1 != cnt) {
            buf += "|\n";
        }
        else {
            buf += "|";
        }
    }
}

#pragma endregion

#pragma region format switch

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<5>) -> decltype(t.format(buf, fmt)) {
    return t.format(buf, fmt);
}

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<4>) -> decltype(t.format(buf)) {
    (void)fmt;
    return t.format(buf);
}

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<3>) -> decltype(formatImpl(buf, fmt, t)) {
    return formatImpl(buf, fmt, t);
}

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<2>) -> $when<$is_base_of<IFormatable, T>> {
    (void)fmt;
    return IFormatable::_format(buf, t);
}

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<1>) -> decltype(static_cast<StrView>(t), 0) {
    auto str = static_cast<StrView>(t);
    formatImpl(buf, fmt, str);
    return 0;
}

template<class Tchar, u32 Usize, class T>
__forceinline auto _format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t, Version<0>) -> $when<$is_enum<T>> {
    auto str = mkEnum(t).name();
    if (str.count() != 0) {
        formatImpl(buf, fmt, str);
    }
    else {
        buf += typeof<T>().name();
        buf += ".";
        formatImpl(buf, fmt, u32(t));
    }
    return;
}

template<class Tchar, u32 Usize, class T>
__forceinline auto format_switch(TString<Tchar, Usize>& buf, const View<const Tchar>& fmt, const T& t) {
    return _format_switch(buf, fmt, t, Version<5>{});
}

template<class T>
String tostr(const T& t) {
    String buf;
    format_switch(buf, StrView{}, t);
    return buf;
}

#pragma endregion

template<class Tchar>
class Formatter
{
public:
    using String  = TString<Tchar>;
    using StrView = View<const Tchar>;

    class EOutOfRange : public IException
    {};

    Formatter(String& buff, const StrView& fmts)
        : buff_(buff), fmts_(fmts)
    {}

    template<class ...U>
    void operator()(const U& ...u) {
        u32     id = 0;
        StrView fmt;

        while (next(id, fmt)) {
            doFormat(id, fmt, u...);
            ++id;
        }
    }

protected:
    String&     buff_;
    StrView     fmts_;

    NMS_API bool next(u32& id, StrView& fmt);

    void doFormat(i32 id, const StrView& fmt) const {
        NMS_THROW(EOutOfRange{});
    }

    template<class T, class ...U>
    void doFormat(i32 id, StrView fmt, const T& t, const U& ...u) {
        if (id == 0) {
            format_switch(buff_, fmt, t);
        }
        else {
            doFormat(id - 1, fmt, u...);
        }
    }
};

template<class ...T>
void sformat(String& buf, const StrView& fmt, const T& ...t) {
    Formatter<char> fmtter(buf, fmt);
    fmtter(t...);
}

/* format to string */
template<class ...T>
auto format(const StrView& fmt, const T& ...t) {
    U8String<1024> buf = {};
    Formatter<char> fmtter(buf, fmt);
    fmtter(t...);
    return buf;
}

}
