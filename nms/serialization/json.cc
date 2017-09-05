#include <nms/serialization/json.h>
#include <nms/io/log.h>
#include <nms/io/console.h>
#include <nms/test.h>

namespace nms::serialization::json
{

// format
void formatNode(String& buf, const NodeEx& node, i32 level=0) {
    StrView fmt;

    auto& v = node.val();

    switch (v.type()) {
    case Type::null:    formatImpl(buf, fmt, "");               break;
    case Type::boolean: formatImpl(buf, fmt, v.bool_val_);      break;
    case Type::u16:     formatImpl(buf, fmt, v.u16_val_);       break;
    case Type::i16:     formatImpl(buf, fmt, v.i16_val_);       break;
    case Type::u32:     formatImpl(buf, fmt, v.u32_val_);       break;
    case Type::i32:     formatImpl(buf, fmt, v.i32_val_);       break;
    case Type::u64:     formatImpl(buf, fmt, v.u64_val_);       break;
    case Type::i64:     formatImpl(buf, fmt, v.i64_val_);       break;
    case Type::f32:     formatImpl(buf, fmt, v.f32_val_);       break;
    case Type::f64:     formatImpl(buf, fmt, v.f64_val_);       break;

    case Type::datetime:
        buf += "\"";
        DateTime(v.i64_val_).format(buf, fmt);
        buf += "\"";
        break;

    case Type::number: {
        formatImpl(buf, fmt, v.str());
        break;
    }

    case Type::key: case Type::string: {
        buf += "\"";
        formatImpl(buf, fmt, v.str());
        buf += "\"";
        break;
    }

    case Type::array: {
        buf += "[\n";

        for (auto itr = node.begin(); itr != node.end(); ) {
            for (auto i = 0; i < (level+1) * 4; ++i) {
                buf += ' ';
            }
            formatNode(buf, *itr, level + 1);
            formatImpl(buf, fmt, (++itr == node.end()) ? StrView{ "\n" } : StrView{ ",\n" });
        }

        for (auto i = 0; i < level * 4; ++i) buf += ' ';
        buf += "]";
        break;
    }
    case Type::object: {
        buf += "{\n";

        for (auto itr = node.begin(); itr != node.end();) {
            for (auto i = 0; i < (level+1) * 4; ++i) {
                buf += ' ';
            }
            buf += "\"";
            buf += itr.key();
            buf += "\": ";
            formatNode(buf, *itr, level + 1);
            formatImpl(buf, fmt, (++itr == node.end()) ? StrView{ "\n" } : StrView{ ",\n" });
        }

        for (auto i = 0; i < level * 4; ++i) buf += ' ';
        buf += "}";
        break;
    }
    default:
        break;
    }
}

NMS_API void formatImpl(String& buf, const NodeEx& tree, StrView/*fmt*/) {
    const auto capicity = tree.count() * 8 + tree.strlen();
    buf.reserve(buf.count()+capicity);
    formatNode(buf, tree, 0);
}

// parse
static bool expect(StrView expect, StrView text) {
    for (u32 i = 1; i < expect.count(); ++i) {
        if (expect[i] != text[i]) {
            return false;
        }
    }
    return true;
}

// test if blank
static bool isBlank(char c) {
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return true;
    return false;
}

static char peekChar(StrView& text) {
    u32 pos = 0;
    u32 len = u32(text.count());
    while (pos < len && isBlank(text[pos])) pos++;

    auto c = text[pos];
    text = text.slice(pos, u32(text.count() - 1));
    return c;
}

static char parseChar(StrView& text) {
    auto c = peekChar(text);
    text=text.slice(1u, u32(text.count()) - 1);
    return c;
}

static i32 parseAny(StrView& text, NodeEx* ptree, i32 proot, i32 pleft);

static i32 parseNum(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    // 0123456789,
    // ^         ^
    // s         p
    auto s = text.data();
    auto p = s;
    while (*p != ',' && *p != '}' && *p != ']' && *p != '\n') {
        ++p;
    }

    auto ret = ptree->add(proot, pleft, Node(StrView{ s, u32(p-s) }, Type::number));
    text     = StrView{p, u32(text.count() - 1)};
    return ret;
}

static i32 parseStr(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    // "abcdefg"
    // ^ ......^
    // b       e
    auto ptr    = text.data();
    auto pos    = text.data() + 1;

    while (true) {
        auto c = *pos;

        if (c == '\\') {
            pos += 2;
        }
        else if (c == '"') {
            break;
        }
        else {
            pos += 1;
        }
    }

    const auto b = 0;
    const auto e = u32(pos-ptr);
    const auto ret = ptree->add(proot, pleft, Node(StrView{ text.data() + b + 1, e - b - 1 }, Type::string));
    text = text.slice(e+1, u32(text.count()) - 1);
    return ret;
}

static i32 parseKey(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    // "abcdefg"
    // ^ ......^
    // b       e

    u32 b = 0;
    u32 e = 1;
    while (text[e] != '"' && text[e - 1] != '\\') ++e;
    auto ret = ptree->add(proot, pleft, Node(StrView{ text.data() + b + 1,  e - b - 1 }, Type::key));
    text = text.slice( e + 1, u32(text.count()) - 1);
    return ret;
}

static i32 parseArray(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    auto parr = ptree->add(proot, pleft, Node(Type::array));

    text = text.slice(1u, u32(text.count()) - 1);
    if (peekChar(text) == ']') {
        parseChar(text);
        return parr;
    }

    auto prev_val = -1;
    while (true) {
        auto this_val = parseAny(text, ptree, parr, prev_val);
        prev_val = this_val;

        auto next_char = parseChar(text);
        if (next_char == ']') {
            return parr;
        }
        if (next_char == ',') {
            continue;
        }
        return -1;
    }
}

static i32 parseObject(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    // add new node
    auto pobj = ptree->add(proot, pleft, Node(Type::object));

    text = text.slice(1, - 1);
    if (peekChar(text) == '}') {
        return pobj;
    }

    auto prev_key = -1;
    auto prev_val = -1;

    while (true) {
        // key
        const auto next_quota  = peekChar(text);
        if (next_quota != '"') {
            return -1;
        }
        const auto this_key  = parseKey(text, ptree, pobj, prev_key);

        const auto next_colon = parseChar(text);
        if (next_colon != ':') {
            io::log::error("nms.serialization.json.parse_object: expect ':', but '{:c}' ", next_colon);
            return -1;
        }
        const auto this_val  = parseAny(text, ptree, -1, prev_val);
        (void)this_val;

        prev_key = this_key;
        prev_val = this_val;

        const auto next_char = parseChar(text);
        if (next_char == '}') {
            break;
        }
        if (next_char == ',') {
            continue;
        }
        io::log::error("nms.serialization.json.parse_object: expect ',' or '}', but '{:c}' ", next_char);
        return -1;
    }
    return pobj;
}

static i32 parseAny(StrView& text, NodeEx* ptree, i32 proot, i32 pleft) {
    auto result = -1;

    const auto text_len    = text.count();

    // peek next char
    const auto next_char = peekChar(text);

    // test char
    switch (next_char) {
        case 'n': {
            if (expect("null", text))  {
                text    = text.slice(4u, text_len - 1);
                result  = ptree->add(proot, pleft, Node{Type::null});
            }
            break;
        }
        case 't': {
            if (expect("true", text)) {
                text    = text.slice(4u, text_len - 1);
                result  = ptree->add(proot, pleft, Node{ true });
            }
            break;
        }
        case 'f': {
            if (expect("false", text)) {
                text    = text.slice(5u, text_len - 1);
                result  = ptree->add(proot, pleft, Node{ false});
            }
            break;
        }

        case '[': {
            result = parseArray(text, ptree, proot, pleft);
            break;
        }

        case '{': {
            result = parseObject(text, ptree, proot, pleft);
            break;
        }

        case '"': {
            result = parseStr(text, ptree, proot, pleft);
            break;
        }

        case '+':
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            result = parseNum(text, ptree, proot, pleft);
            break;
        }

        default:
            break;
    }

    return result;
}

// wraper
NMS_API Tree parse(StrView text) {
    Tree tree;
    tree.reserve(text.count() / 10);
    parseAny(text, &tree, -1, -1);
    return tree;
}

}

#pragma region unittest

namespace nms::serialization::json
{

struct TestObject
    : public IFormatable
    , public ISerializable
{
    NMS_PROPERTY_BEGIN;
    typedef U8String<32>    NMS_PROPERTY(a);
    typedef i32x3           NMS_PROPERTY(b);
    typedef DateTime        NMS_PROPERTY(c);
    NMS_PROPERTY_END;
};


nms_test(serialization) {
    TestObject obj;
    obj.a = "hello";
    obj.b = { 1, 2, 3 };
    obj.c = DateTime(2017, 9, 3, 8, 30, 12);
    auto jstr = json::format(obj);
    io::console::writeln("json = {}", jstr);
}

nms_test(deserialization) {
    const char text[] = R"(
{
    "a": "hello",
    "b": [ 1, 2, 3],
    "c": "2017-9-3T8:30:12"
}
)";
    // json_str -> json_tree
    auto obj = json::parse(text);

    TestObject val;
    obj >> val;

    io::console::writeln("obj = {}", val);
}

}

#pragma endregion
