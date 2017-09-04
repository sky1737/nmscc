
#include <nms/test.h>
#include <nms/math.h>
#include <nms/io.h>

namespace nms::math
{

nms_test(array_basic) {
    Array<f32, 2> a({ 4, 4 });
    a <<= lins(1.0f, 0.1f);

    io::console::writeln("a = {:+6.3}", a);

    Array<f32, 2> b;
    b.resize({ 4, 4 });
    b <<= a;
    io::console::writeln("b = {:+6.3}", b);

    auto c = b.permute({ 1u, 0u });
    io::console::writeln("c = {:+6.3}", c);

    auto pd = mnew<f32>(c.count());
    Array<f32, 2> d(pd, c.size(), [=] {mdel(pd); });
    d <<= c;
    io::console::writeln("d = {:-6.3}", d);

    auto e = move(d);
    io::console::writeln("e = {:-6.3}", e);
}

nms_test(array_math) {
    // a = zeros(32, 32)

    // b = zeros(32, 32)
    Array<f32, 2> a({ 32u, 32u });
    Array<f32, 2> b({ 32u, 32u });

    // b = |  0.0   0.1   0.2   0.3 ...  1.0 ...
    //     |  1.0   1.1   1.2   1.3 ...  1.1 ...
    //     | ...                             ...
    //     | 10.0  10.1  10.2  10.3 ... 11.0 ...
    //     | ...                             ...
    b <<= lins(0.1f, 1.f);

    // a = sin(b).*2 + cos(b)
    a <<= sin(b) * 2 + cos(b);

    // h:= host memory
    math::Array<f32, 2> h({ 32u, 32u });

    // h = a  (copy from device to host)
    h <<= a;

    io::console::writeln("result = {:-6.3}", h.slice({ 0u, 8u }, { 0u, 8u }));
}

nms_test(array_project3d) {
    Array<f32, 3> imag({ 10u, 64u, 64u });
    Array<f32, 2> view({ 64u, 64u });

    imag <<= lins(0.f, 0.01f, 1.f);
    view <<= sum(imag);

    auto x_imag = imag.slice({ 0u }, { 0u, 8u }, { 0u, 8u });
    auto x_view = view.slice({ 0u, 8u }, { 0u, 8u });

    io::console::writeln("imag = {:-7.3}", x_imag);
    io::console::writeln("view = {:-7.3}", x_view);
}

}
