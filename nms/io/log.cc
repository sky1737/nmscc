#include <nms/core.h>
#include <nms/io/log.h>
#include <nms/io/console.h>
#include <nms/io/file.h>
#include <nms/thread/mutex.h>
#include <nms/util/stacktrace.h>

namespace nms::io::log
{

using namespace nms::thread;

Level   gLevel = Level::None;

NMS_API String& gStrBuf() {
    static TString<char, 4096> buf;
    return buf;
}

NMS_API Level getLevel() {
    return gLevel;
}

NMS_API void setLevel(Level level) {
    gLevel = level;
}

static StrView to_str(Level level) {
    switch (level) {
    case Level::None:   return "none";
    case Level::Debug:  return "debug";
    case Level::Info:   return "info";
    case Level::Warn:   return "warning";
    case Level::Alert:  return "alert";
    case Level::Error:  return "error";
    case Level::Fatal:  return "fatal";
    }
    return "unknow";
}

class LogFile
{
public:
    LogFile()
    {}

    ~LogFile() {
        if (txtfile_ == nullptr) {
            return;
        }
        close();
    }

    operator bool() const {
        return txtfile_ != nullptr;
    }

    void open(const Path& path) {
        if (txtfile_ != nullptr) {
            close();
        }
        if (path.str().isEmpty()) {
            return;
        }

        txtfile_ = new io::TxtFile{ path, File::Write };
    }

    void close() {
        if (txtfile_ == nullptr) {
            return;
        }
        delete txtfile_;
    }

    void write(Level level, f64 time, StrView message) {
        if (txtfile_ == nullptr) {
            return;
        }

        LockGuard lock_guard(mutex_);
        char buff[1024];
        auto buff_len = snprintf(buff, sizeof(buff), "%8.3f (%s) ", time, to_str(level).data());
        txtfile_->write(StrView(buff, { u32(buff_len) }));
        txtfile_->write(message);
        txtfile_->sync();
    }

protected:
    Mutex       mutex_;
    TxtFile*    txtfile_    = nullptr;
};

LogFile gLogFile = {};

NMS_API void setLogPath(const Path& path) {
    gLogFile.open(path);
}

NMS_API void message(Level level, StrView msg) {

    if (level < gLevel) {
        return;
    }

    // current process time
    const auto time = clock();

    // 1. terminal
    {
        static const StrView titles[]   ={
            "[  ]",
            "\033[1;1m[--]",
            "\033[1;32m[**]",                     // green
            "\033[1;33m[??]",    "\033[1;43m[??]",// yellow
            "\033[1;31m[!!]",    "\033[1;41m[!!]" // red
        };
        const auto& title       = titles[u32(level)];
        char        head[64];
        auto        head_len    = snprintf(head, sizeof(head), "%*s%7.3f\033[0m ", int(title.count()), title.data(), time);


        StrView strs[] = {
            StrView(head, {u32(head_len)}),
            StrView(msg),
            StrView("\n")
        };
        console::writes(strs, numel(strs));

        if (level >= Level::Fatal) {
            CallStacks stacks;
            const auto n = stacks.count() - 4;
            for (u32 i = 0; i < n; ++i) {
                console::writeln("\t|-[{}] {}", i, stacks[i]);
            }
        }
    }

    // 2. xml
    if (gLogFile) {
        gLogFile.write(level, time, msg);
    }
}

}
