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

    // Specifying the Big Endian format
    template <char FormatChar>
    struct BigEndianFormat {
        static_assert(isFormatChar(FormatChar), "Invalid Format Char passed");
        static constexpr size_t size() {
            return 0;
        }
    };
#undef BigEndianFormat
#define SET_FORMAT_CHAR(ch, s, rep_type, native_rep_type)                      \
    template <>                                                                \
    struct BigEndianFormat<ch> {                                               \
        static constexpr size_t size() {                                       \
            return s;                                                          \
        }                                                                      \
        static constexpr size_t nativeSize() {                                 \
            return sizeof(native_rep_type);                                    \
        }                                                                      \
        using RepresentedType = rep_type;                                      \
        using NativeRepresentedType = native_rep_type;                         \
    }
    template <typename Fmt, char FormatChar>
    using RepresentedType = std::conditional_t<
        Fmt::isNative(),
        typename BigEndianFormat<FormatChar>::NativeRepresentedType,
        typename BigEndianFormat<FormatChar>::RepresentedType>;
    SET_FORMAT_CHAR('?', 1, bool, bool);
    SET_FORMAT_CHAR('x', 1, char, char);
    SET_FORMAT_CHAR('b', 1, int8_t, signed char);
    SET_FORMAT_CHAR('B', 1, uint8_t, unsigned char);
    SET_FORMAT_CHAR('c', 1, char, char);
#undef SET_FORMAT_CHAR

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

        struct RawFormatType {
            char           formatChar;
            size_t         repeat;
            constexpr auto isString() -> bool {
                return formatChar == 's';
            }
        };
        struct FormatType {
            char           formatChar;
            size_t         formatSize;
            size_t         size;
            constexpr auto isString() -> bool {
                return formatChar == 's';
            }
        };
        constexpr auto doesFormatAlign(FormatType format) -> bool {
            return format.formatSize > 1;
        }
        template <typename Fmt, char FormatChar, size_t Repeat = 1>
        constexpr auto getSize() -> size_t {
            if constexpr (format_mode().is_native()) {
                return BigEndianFormat<FormatChar>::nativeSize() * Repeat;
            } else {
                return BigEndianFormat<FormatChar>::size() * Repeat;
            }
        }

        template <size_t Item, typename Fmt, size_t... Is>
        constexpr auto getTypeOfItem(std::index_sequence<Is...> /*unused*/)
            -> RawFormatType {
            constexpr char fomratString[] = {Fmt::at(Is)...};
            RawFormatType  wrappedTypes[countItems(Fmt{})]{};
            size_t         currentType = 0;
            for (size_t i = 0; i < sizeof...(Is); i++) {
                if (is_format_mode(fomratString[i])) {
                    continue;
                }
                auto repeatCount = consume_number(fomratString, i);
                i = repeatCount.second;
                wrappedTypes[currentType].formatChar = fomratString[i];
                wrappedTypes[currentType].repeat = repeatCount.first;
                if (repeatCount.first == 0) {
                    wrappedTypes[currentType].repeat = 1;
                }
                currentType++;
            }
            return getUnwrappedItem<Item>(wrappedTypes);
        }

        template <size_t Item, typename Fmt>
        constexpr auto getTypeOfItem() -> FormatType {
            static_assert(Item < count_items(),
                          "Item requested must be inside the format");
            constexpr RawFormatType format = getTypeOfItem<Item, Fmt>(
                std::make_index_sequence<Fmt::size()>());
            constexpr FormatType type
                = {format.formatChar,
                   getSize<Fmt, format.formatChar>(),
                   getSize<Fmt, format.formatChar, format.repeat>()};
            return type;
        }

        template <typename Fmt, size_t... Items>
        constexpr auto getBinaryOffset(std::index_sequence<Items...> /*unused*/)
            -> size_t {
            constexpr FormatType itemTypes[] = {getTypeOfItem<Items>(Fmt{})...};
            constexpr auto       formatMode = format_mode();
            size_t               size = 0;
            for (size_t i = 0; i < sizeof...(Items) - 1; i++) {
                size += itemTypes[i].size;
                if (formatMode.shouldPad()) {
                    if (doesFormatAlign(itemTypes[i + 1])) {
                        auto currentAlignment
                            = (size % itemTypes[i + 1].formatSize);
                        if (currentAlignment != 0) {
                            size += itemTypes[i + 1].formatSize
                                    - currentAlignment;
                        }
                    }
                }
            }
            return size;
        }

        template <size_t Item, typename Fmt>
        constexpr auto getBinaryOffset() -> size_t {
            return getBinaryOffset(std::make_index_sequence<Item + 1>());
        }

        // https://docs.python.org/3/library/struct.html#struct.calcsize
        constexpr auto calcsize() -> std::size_t {
            // 3H4B10s:
            // count_items: 3*4 + 4 * 1 + 10
            constexpr auto num_items = count_items();
            constexpr auto last_item = getTypeOfItem<num_items - 1>();
            return getBinaryOffset<num_items - 1>() + last_item.size;
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
