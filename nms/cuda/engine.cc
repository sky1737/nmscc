#include <nms/core.h>
#include <nms/io.h>
#include <nms/test.h>
#include <nms/util/library.h>

#include <nms/cuda/runtime.h>
#include <nms/cuda/engine.h>
#include <nms/cuda/array.h>
#include <nms/cuda/kernel.h>

extern "C"
{
    typedef struct _nvrtcProgram *nvrtcProgram;

    enum nvrtcResult
    { };

    const char *nvrtcGetErrorString(nvrtcResult result);

    nvrtcResult nvrtcCreateProgram(nvrtcProgram *prog, const char *src, const char *name, int numHeaders, const char * const *headers, const char * const *includeNames);
    nvrtcResult nvrtcDestroyProgram(nvrtcProgram *prog);

    nvrtcResult nvrtcCompileProgram(nvrtcProgram prog, int numOptions, const char * const *options);

    nvrtcResult nvrtcGetProgramLogSize(nvrtcProgram prog, size_t *logSizeRet);
    nvrtcResult nvrtcGetProgramLog(nvrtcProgram prog, char *log);

    nvrtcResult nvrtcGetPTXSize(nvrtcProgram prog, size_t *ptxSizeRet);
    nvrtcResult nvrtcGetPTX(nvrtcProgram prog, char *ptx);
}

namespace nms::cuda
{

#pragma region library

#define NMS_NVRTC_DEF(F)                                                            \
    F(nvrtcCreateProgram),      F(nvrtcDestroyProgram),     F(nvrtcCompileProgram), \
    F(nvrtcAddNameExpression),  F(nvrtcGetLoweredName),                             \
    F(nvrtcGetProgramLog),      F(nvrtcGetProgramLogSize),                          \
    F(nvrtcGetPTX),             F(nvrtcGetPTXSize)
    
enum NVRTCTable
{
#define NMS_NVRTC_IDX(name) $##name
        NMS_NVRTC_DEF(NMS_NVRTC_IDX)
#undef  NMS_NVRTC_IDX
};

static Library::Function nvrtcFun(u32 id) {
    static Library lib("nvrtc64_80.dll");

    static Library::Function funcs[] = {
#define NMS_NVRTC_FUN(name) lib[StrView{#name}]
        NMS_NVRTC_DEF(NMS_NVRTC_FUN)
#undef  NMS_NVRTC_FUN
    };

    auto ret = funcs[id];
    return ret;
}

#define NMS_NVRTC_DO(name)   static_cast<decltype(name)*>(nvrtcFun($##name))

#pragma endregion

#pragma region program

void driver_init();

NMS_API Program::Program(StrView src)
    : src_{}
    , ptx_{} 
{
    const auto init_size = 1024u * 1024u;
    const auto buff_size = max(init_size, src.count());
    src_.reserve(buff_size);
    src_ += src;
}


NMS_API Program::~Program() 
{}

NMS_API void Program::addSrc(StrView src) {
    src_ += src;
}

NMS_API bool Program::compile() {
    static const char*  argv[] = { "--std=c++11", "-default-device", "-restrict", "--use_fast_math", "-arch=compute_30"};
    static const int    argc = sizeof(argv) / sizeof(argv[0]);

    // create program
    nvrtcProgram nvrtc = nullptr;
    NMS_NVRTC_DO(nvrtcCreateProgram)(&nvrtc, src_.cstr(), nullptr, 0, nullptr, nullptr);
    if (nvrtc == nullptr) {
        io::log::error("nms.cuda.Program: cannot create program.");
    }

    // compile
    auto ret = NMS_NVRTC_DO(nvrtcCompileProgram)(nvrtc, argc, argv);

    if (int(ret) != 0) {
        size_t nvrtc_log_size = 0;
        NMS_NVRTC_DO(nvrtcGetProgramLogSize)(nvrtc, &nvrtc_log_size);

        String nvrtc_log;
        nvrtc_log.resize(u32(nvrtc_log_size));
        NMS_NVRTC_DO(nvrtcGetProgramLog)(nvrtc, nvrtc_log.data());
        io::console::writeln(nvrtc_log);

        return false;
    }

    // get ptx
    size_t ptx_len = 0;
    NMS_NVRTC_DO(nvrtcGetPTXSize)(nvrtc, &ptx_len);

    ptx_.reserve(u32(ptx_len));
    ptx_.resize(u32(ptx_len)-1);
    NMS_NVRTC_DO(nvrtcGetPTX)(nvrtc, ptx_.data());

    // destroy program
    NMS_NVRTC_DO(nvrtcDestroyProgram)(&nvrtc);

    return true;
}
#pragma endregion

NMS_API Program& Texec::sProgram() {
    static Program program(nms_cuda_kernel_src);
    return program;
}

NMS_API Module& Texec::sModule() {
    static StrView ptx_src = [=] {
        const io::Path src_path("nms.cuda.kernel.cu");
        const io::Path ptx_path("nms.cuda.kernel.ptx");
        auto& program = sProgram();

        // if not exists, try save
        if (!io::exists(ptx_path)) {
            auto stat = program.compile();

            auto src = program.src();
            if (!stat && src.count() > 0) {
                io::TxtFile src_file(src_path, io::File::Write);
                src_file.write(src);
            }

            auto ptx = program.ptx();
            if (stat && ptx.count() > 0) {
                io::TxtFile ptx_file(ptx_path, io::File::Write);
                ptx_file.write(ptx);
            }
        }

        static auto ptx = io::loadString(ptx_path);
        return StrView(ptx);
    }();

    static Module value(ptx_src);
    return value;
}

NMS_API Module::fun_t Texec::_get_kernel(u32 fid) {
    static auto& mod = sModule();

    char name[64];
    auto count = snprintf(name, sizeof(name), "nms_cuda_foreach_%u", fid);
    auto func  = mod.get_kernel(StrView(name, u32(count)));
    return func;
}

NMS_API u32 Texec::_add_func(StrView func, StrView ret_type, StrView arg_type) {
    static auto func_id = 0;

    U8String<1024> src;

    sformat(src, "__kernel__ void nms_cuda_foreach_{}(\n", func_id);
    sformat(src, "    {} ret,\n", ret_type);
    sformat(src, "    {} arg)\n", arg_type);
    src += "{\n";
    sformat(src, "    nms::cuda::foreach<{}>(ret, arg);\n", func);
    src += "}\n\n";

    sProgram().addSrc(src);

    return func_id++;
}

namespace engine
{

NMS_API Program& gProgram() {
    static Program program(nms_cuda_kernel_src);
    return program;
}

NMS_API Module&  gModule() {
    static StrView ptx_src = [=] {
        const io::Path ptx_path("nms.cuda.program.ptx");
        auto& program = gProgram();

        // if not exists, try save
        if (!io::exists(ptx_path)) {
            const auto stat = program.compile();
            (void)stat;

            const auto ptx = program.ptx();
            if (ptx.count() > 0) {
                io::TxtFile ptx_file(ptx_path, io::File::Write);
                ptx_file.write(ptx);
            }
        }

        static auto ptx = io::loadString(ptx_path);
        return StrView(ptx);
    }();

    static Module value(ptx_src);
    return value;
}

}

}


#pragma region unittest
void nms_cuda_Engine_test(nms::View<nms::f32, 2> v)
{}

static const char nms_cuda_Engine_test_src[] = R"(
__kernel__ void nms_cuda_Engine_test(nms::View<nms::f32,2> v) {
    const auto ix = blockIdx.x*blockDim.x+threadIdx.x;
    const auto iy = blockIdx.y*blockDim.y+threadIdx.y;
    v(ix, iy) = ix+iy*0.1f;
})";

namespace nms::cuda
{

nms_test(Engine) {
    engine::add_src(nms_cuda_Engine_test_src);

    cuda::Array<f32, 2> dv({ 16, 16 });
    nms_cuda_kfunc(nms_cuda_Engine_test)[{ 16u, 16u }](dv);

    host::Array<f32, 2> hv(dv.size());
    hv <<= dv;
    io::console::writeln("v = {|}", hv.slice({ 0u, 8u }, { 0u, 8u }));
}

}

#pragma endregion




