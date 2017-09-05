
#include <typeinfo>
#include <nms/core/exception.h>
#include <nms/core/string.h>
#include <nms/core/format.h>
#include <nms/io/log.h>
#include <nms/util/stacktrace.h>

namespace nms
{

NMS_API const CallStacks& gExceptionStacks() {
    static thread_local CallStacks stacks;
    return stacks;
}

NMS_API void setExceptionStacks(CallStacks&& stack) {
    auto& gstack = const_cast<CallStacks&>(gExceptionStacks());
    gstack = static_cast<CallStacks&&>(stack);
}

NMS_API void dump(const IException& e)  {
    auto  cname  = typeid(e).name();
    auto  name   = StrView{ cname, u32(strlen(cname)) };
    auto& stacks = gExceptionStacks();

    U8String<1024> buff;
    sformat(buff, "throw {}: {}\n", name, e);
   
    const auto stacks_cnt = stacks.count();
    for (auto i = 0u; i < stacks_cnt; ++i) {
        (i + 1 != stacks_cnt)
            ? sformat(buff, StrView(u8"\t ├─{:2}: {}\n"), i, stacks[i])
            : sformat(buff, StrView(u8"\t └─{:2}: {}"), i, stacks[i]);
    }
    io::log::error(buff);
}

NMS_API u32 ESystem::current() {
    return errno;
}

NMS_API void ESystem::format(String& buf) const {
    char tmp[256] = { "" };
    auto ptr = tmp;
#if defined(NMS_OS_WINDOWS)
    strerror_s(static_cast<char*>(tmp), sizeof(tmp), id_);
#elif defined(NMS_OS_APPLE)
    // XSI version
    const auto ret = strerror_r(id_, tmp, sizeof(tmp));
    if (ret != 0) {
        return;
    }
#elif defined(NMS_OS_UNIX)
    // GNU version
    ptr = strerror_r(id_, tmp, sizeof(tmp));
#endif
    StrView str{ ptr, u32(strlen(ptr)) };
    sformat(buf, "system error({}): {}", id_, str);
}

}
