#pragma once

#include <nms/core/string.h>

namespace nms::thread
{
class Mutex;
}

namespace nms::io::console
{

NMS_API void goto_line(i32 line);
NMS_API void progress_bar(f32 percent);
NMS_API void writes(const StrView texts[], u32 n);

inline String& gStrBuff() {
    static String str;

    static auto _init = [&] {
        str.reserve(16384); // 16K
    };
    (void)_init;

    return str;
}

inline void  write(StrView text) {
    StrView texts[] = { text };
    writes(texts, 1);
}

inline void  writeln(StrView text) {
    StrView texts[] = { text, "\n" };
    writes(texts, 2);
}

template<class T, class ...U>
void write(StrView fmt, const T& t, const U& ...u) {
    auto& buf = gStrBuff();
    buf.resize(0);
    sformat(buf, fmt, t, u...);
    write(buf);
}


template<class T, class ...U>
void writeln(StrView fmt, const T& t, const U& ...u) {
    auto& buf = gStrBuff();
    buf.resize(0);
    sformat(buf, fmt, t, u...);
    writeln(buf);
}

}
