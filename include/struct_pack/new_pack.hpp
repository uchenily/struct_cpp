#include "struct_pack/debug.hpp"
#include "struct_pack/string_literal.hpp"
#include <tuple>

namespace struct_pack {
namespace detail {
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

    template <auto container>
    struct fmt_string {
        static constexpr auto data() -> const char * {
            return container.data;
        }

        static constexpr auto size() -> std::size_t {
            return container.size();
        }

        static constexpr auto view() -> std::string_view {
            return std::string_view{data(), size()};
        }

        static constexpr auto at(std::size_t i) -> char {
            // static_assert(i < size());
            return data()[i];
        }

        static constexpr auto is_format_mode(char ch) -> bool {
            return ch == '<' || ch == '>' || ch == '!' || ch == '='
                   || ch == '@';
        }

        static constexpr auto is_digit(char ch) -> bool {
            return ch >= '0' && ch <= '9';
        }

        static constexpr auto consume_number(size_t offset)
            -> std::pair<size_t, size_t> {
            size_t num = 0;
            size_t i = offset;
            for (; is_digit(at(i)) && i < container.size(); i++) {
                num = static_cast<size_t>(num * 10 + (at(i) - '0'));
            }

            return {num, i - 1};
        }

        static constexpr auto count_items() -> size_t {
            size_t item_count = 0;
            size_t num = 1;
            for (size_t i = 0; i < size(); i++) {
                auto current = at(i);
                if (i == 0 && is_format_mode(current)) {
                    continue;
                }

                if (is_digit(current)) {
                    std::tie(num, i) = consume_number(i);
                    continue;
                }

                if (current == 's') {
                    item_count++;
                } else {
                    item_count += num;
                }
                num = 1;
            }

            return item_count;
        }

        static constexpr auto is_format_char(char formatChar) -> bool {
            return is_format_mode(formatChar) || formatChar == 'x'
                   || formatChar == 'b' || formatChar == 'B'
                   || formatChar == 'c' || formatChar == 's'
                   || formatChar == 'h' || formatChar == 'H'
                   || formatChar == 'i' || formatChar == 'I'
                   || formatChar == 'l' || formatChar == 'L'
                   || formatChar == 'q' || formatChar == 'Q'
                   || formatChar == 'f' || formatChar == 'd'
                   || formatChar == '?' || is_digit(formatChar);
        }

        constexpr auto format_mode() {
            if constexpr (is_format_mode(at(0))) {
                constexpr auto first_char = at(0);
                return new_FormatMode<first_char>{};
            } else {
                return new_FormatMode<'@'>{};
            }
        }
    };

    template <string_container container,
              std::size_t      I,
              std::size_t      N,
              typename... Args>
    auto pack_helper(Args... args) {
        if constexpr (I < N) {
            pack_helper<container, I + 1, N>(std::forward<Args>(args)...);
        }
    }
} // namespace detail

template <string_container container, typename... Args>
auto new_pack(Args... args) {
    auto fmt = detail::fmt_string<container>{};

    constexpr size_t N = fmt.count_items();
    static_assert(N == sizeof...(args));

    constexpr auto format_mode = fmt.format_mode();
    PRINT("format mode: {}", format_mode.format());
    detail::pack_helper<container, 0, N>(std::forward<Args>(args)...);

    return fmt;
}
} // namespace struct_pack
