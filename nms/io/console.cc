#include <nms/core.h>
#include <nms/io/console.h>
#include <nms/thread.h>
#include <nms/test.h>

extern "C"
{
    using namespace nms;

    void* GetStdHandle(long fildno);
    int   SetConsoleMode(void*, unsigned long);
}

namespace nms::io::console
{

static auto _init_console() {
#ifdef NMS_OS_WINDOWS
    auto hout = ::GetStdHandle(-11);
    ::SetConsoleMode(hout, 0xF);
    ::system("chcp 65001"); // windows cannot set local to utf-8
#endif
    return 0;
}

NMS_API void writes(const StrView text[], u32 n) {
    static auto init = _init_console();
    (void)init;

    static thread::Mutex _mutex;
    thread::LockGuard lock(_mutex);
    for (u32 i = 0; i < n; ++i) {
        auto dat = text[i].data();
        auto len = text[i].count();
        if (len > 0) {
            fwrite(dat, 1, len, stdout);
        }
    }
    fflush(stdout);
}

NMS_API bool isterm() {
    static const auto cond = ::isatty(1);
    return cond != 0;
}

NMS_API void goto_line(i32 line) {
    static const char next_cmd[] = "\033[%dE";
    static const char prev_cmd[] = "\033[%dF";

    if (line == 0 || !isterm() ) {
        return;
    }

    ::printf(line > 0 ? next_cmd : prev_cmd, abs(line));
    ::fflush(stdout);
}

NMS_API void show_cursor(bool value) {
    static const char show_cmd[] = "\033[?25h";
    static const char hide_cmd[] = "\033[?25l";

    static auto cond = true;
    if (cond == value || !isterm()) {
        return;
    }
    cond = value;
    fprintf(stdout, "%s", cond ? show_cmd : hide_cmd);
    fflush(stdout);
}

NMS_API void hide_cursor(bool value) {
    show_cursor(!value);
}

NMS_API void progress_bar(f32 percent) {
    static const char digits[]      = u8"█▌░";
    static thread_local auto id     = 0u;

    if (!isterm()) {
        return;
    }

    id = (id+1)%10;

    const u16   prog_size = 80;
    const auto  cnts      = (percent+0.001f)*prog_size;

    if (cnts< prog_size ) {
        // hide cursor
        show_cursor(false);

    }
    else if(cnts == prog_size) {
        // show cursor
        show_cursor(true);
    
    }


    char prog_bar[1024];
    u32  real_cnt = 0;

    prog_bar[real_cnt++] = '\r';
    for (u16 i = 0; i < prog_size; ++i) {
        if (cnts > i) {
            prog_bar[real_cnt++] = digits[0];
            prog_bar[real_cnt++] = digits[1];
            prog_bar[real_cnt++] = digits[2];
        }
        else if (cnts > i - 0.5f) {
            prog_bar[real_cnt++] = digits[3];
            prog_bar[real_cnt++] = digits[4];
            prog_bar[real_cnt++] = digits[5];
        }
        else {
            prog_bar[real_cnt++] = digits[6];
            prog_bar[real_cnt++] = digits[7];
            prog_bar[real_cnt++] = digits[8];
        }
    }
    prog_bar[real_cnt++] = ' ';
    prog_bar[real_cnt++] = ' ';

    real_cnt += snprintf(prog_bar + real_cnt, 10, "%.2f%%", percent*100.f);
    fwrite(prog_bar, real_cnt, 1, stdout);
}

nms_test(progress_bar) {
    const auto cnt = 1000;
    for (auto i = 0; i < cnt; ++i) {
        const auto percent = f32(i) / f32(cnt);
        progress_bar(percent);
    }
    progress_bar(1.0f);
    printf("\n");
}

}

