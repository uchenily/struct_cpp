#pragma once

#include <cstdint>
#include <type_traits>

namespace struct_pack::detail {

constexpr auto is_digit(char ch) -> bool {
    return ch >= '0' && ch <= '9';
}

constexpr auto is_format_mode(char ch) -> bool {
    return ch == '<' || ch == '>' || ch == '!' || ch == '=' || ch == '@';
}

constexpr auto is_format_char(char ch) -> bool {
    return is_format_mode(ch) || ch == 'x' || ch == 'b' || ch == 'B'
           || ch == 'c' || ch == 's' || ch == 'h' || ch == 'H' || ch == 'i'
           || ch == 'I' || ch == 'l' || ch == 'L' || ch == 'q' || ch == 'Q'
           || ch == 'f' || ch == 'd' || ch == '?' || detail::is_digit(ch);
}

// Specifying the format mode
template <char FormatChar>
struct new_FormatMode {
    static constexpr auto is_big_endian() -> bool {
        return false;
    }
    static constexpr auto should_pad() -> bool {
        return false;
    }
    static constexpr auto is_native() -> bool {
        return false;
    }
    static constexpr auto format() -> char {
        return '?';
    }
};
#undef SET_FORMAT_MODE
#define SET_FORMAT_MODE(mode, padding, big_endian, native)                     \
    template <>                                                                \
    struct new_FormatMode<mode> {                                              \
        static constexpr auto is_big_endian() -> bool {                        \
            return big_endian;                                                 \
        }                                                                      \
        static constexpr auto should_pad() -> bool {                           \
            return padding;                                                    \
        }                                                                      \
        static constexpr auto is_native() -> bool {                            \
            return native;                                                     \
        }                                                                      \
        static constexpr auto format() -> char {                               \
            return mode;                                                       \
        }                                                                      \
    }
SET_FORMAT_MODE('@', true, false, true);
SET_FORMAT_MODE('>', false, true, false);
SET_FORMAT_MODE('!', false, true, false);
#undef SET_FORMAT_MODE

// Specifying the Big Endian format
template <char FormatChar>
struct BigEndianFormat {
    static_assert(is_format_char(FormatChar), "Invalid Format Char passed");
    static constexpr auto size() -> std::size_t {
        return 0;
    }
};

#undef SET_FORMAT_CHAR
#define SET_FORMAT_CHAR(ch, s, rep_type, native_rep_type)                      \
    template <>                                                                \
    struct BigEndianFormat<ch> {                                               \
        static constexpr auto size() -> std::size_t {                          \
            return s;                                                          \
        }                                                                      \
        static constexpr auto native_size() -> std::size_t {                   \
            return sizeof(native_rep_type);                                    \
        }                                                                      \
        using RepresentedType = rep_type;                                      \
        using NativeRepresentedType = native_rep_type;                         \
    };

SET_FORMAT_CHAR('?', 1, bool, bool);
SET_FORMAT_CHAR('x', 1, char, char);
SET_FORMAT_CHAR('b', 1, int8_t, signed char);
SET_FORMAT_CHAR('B', 1, uint8_t, unsigned char);
SET_FORMAT_CHAR('c', 1, char, char);
#undef SET_FORMAT_CHAR

template <typename FormatMode, char FormatChar>
using RepresentedType = std::conditional_t<
    FormatMode::is_native(),
    typename BigEndianFormat<FormatChar>::NativeRepresentedType,
    typename BigEndianFormat<FormatChar>::RepresentedType>;
} // namespace struct_pack::detail
