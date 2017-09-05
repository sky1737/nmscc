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

NMS_API void show_cursor(bool value = true);
NMS_API void hide_cursor(bool value = true);

NMS_API void writes(const StrView texts[], u32 n);

namespace
{
static const StrView $rst = "\033[0m";
static const StrView $bld = "\033[1m";
static const StrView $clr = "\033[2J";

static const StrView $fg_blk = "\033[30m";
static const StrView $fg_red = "\033[31m";
static const StrView $fg_grn = "\033[32m";
static const StrView $fg_yel = "\033[33m";
static const StrView $fg_blu = "\033[34m";
static const StrView $fg_mag = "\033[35m";
static const StrView $fg_cyn = "\033[36m";
static const StrView $fg_wht = "\033[37m";

static const StrView $bg_blk = "\033[40m";
static const StrView $bg_red = "\033[41m";
static const StrView $bg_grn = "\033[42m";
static const StrView $bg_yel = "\033[43m";
static const StrView $bg_blu = "\033[44m";
static const StrView $bg_mag = "\033[45m";
static const StrView $bg_cyn = "\033[46m";
static const StrView $bg_wht = "\033[47m";
}

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
