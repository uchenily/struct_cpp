#include "struct_pack/data_view.hpp"
#include "struct_pack/debug.hpp"
#include "struct_pack/format.hpp"
#include "struct_pack/string_literal.hpp"
#include <tuple>
#include <utility>

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
        static constexpr auto size() -> size_t {
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
        static constexpr auto nativeSize() -> std::size_t {                    \
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

            return {num, i};
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
                    i--; // to combat the i++ in the loop
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

        static constexpr auto format_mode() {
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

        static constexpr auto doesFormatAlign(FormatType format) -> bool {
            return format.formatSize > 1;
        }

        template <char FormatChar, size_t Repeat = 1>
        static constexpr auto getSize() -> size_t {
            if constexpr (format_mode().is_native()) {
                return BigEndianFormat<FormatChar>::nativeSize() * Repeat;
            } else {
                return BigEndianFormat<FormatChar>::size() * Repeat;
            }
        }

        template <size_t Item, size_t ArrSize>
        static constexpr auto
        getUnwrappedItem(RawFormatType (&wrappedFormats)[ArrSize])
            -> RawFormatType {
            size_t currentItem = 0;
            for (size_t i = 0; i < ArrSize; i++) {
                for (size_t repeat = 0; repeat < wrappedFormats[i].repeat;
                     repeat++) {
                    auto currentType = wrappedFormats[i];
                    if (currentItem == Item) {
                        if (!currentType.isString()) {
                            currentType.repeat = 1;
                        }
                        return currentType;
                    }
                    currentItem++;
                    if (currentType.isString()) {
                        break;
                    }
                }
            }
            // cannot get here, Item < ArrSize
            return {0, 0};
        }

        template <size_t Item, size_t... Is>
        static constexpr auto
        getTypeOfItem_helper(std::index_sequence<Is...> /*unused*/)
            -> RawFormatType {
            constexpr char fomratString[] = {at(Is)...};
            RawFormatType  wrappedTypes[count_items()]{};
            size_t         currentType = 0;
            for (size_t i = 0; i < sizeof...(Is); i++) {
                if (is_format_mode(fomratString[i])) {
                    continue;
                }
                auto repeatCount = consume_number(i);
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

        template <size_t Item>
        static constexpr auto type_of_item() -> FormatType {
            static_assert(Item < count_items(),
                          "Item requested must be inside the format");
            constexpr RawFormatType format = getTypeOfItem_helper<Item>(
                std::make_index_sequence<size()>());
            constexpr FormatType sizedFormat
                = {format.formatChar,
                   getSize<format.formatChar>(),
                   getSize<format.formatChar, format.repeat>()};
            return sizedFormat;
        }

        template <size_t... Items>
        static constexpr auto
        binary_offset_helper(std::index_sequence<Items...> /*unused*/)
            -> size_t {
            constexpr FormatType itemTypes[] = {type_of_item<Items>()...};
            constexpr auto       formatMode = format_mode();
            size_t               size = 0;
            for (size_t i = 0; i < sizeof...(Items) - 1; i++) {
                size += itemTypes[i].size;
                if (formatMode.should_pad()) {
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
        template <size_t Item>
        static constexpr auto binary_offset() -> size_t {
            return binary_offset_helper(std::make_index_sequence<Item + 1>());
        }

        // https://docs.python.org/3/library/struct.html#struct.calcsize
        static constexpr auto calcsize() -> std::size_t {
            // 3H4B10s:
            // count_items: 3*4 + 4 * 1 + 10 = 16
            // with padding: 3 * 4 + 4*1 + (4bytes) + 10 = 20
            constexpr auto num_items = count_items();
            constexpr auto last_item = type_of_item<num_items - 1>();
            return binary_offset<num_items - 1>() + last_item.size;
        }

        template <typename RepType>
        static constexpr auto pack_element(char      *data,
                                           bool       bigEndian,
                                           FormatType format,
                                           RepType    elem) -> int {
            if constexpr (std::is_same_v<RepType, std::string_view>) {
                // Trim the string size to the repeat count specified in the
                // format
                elem = std::string_view(elem.data(),
                                        std::min(elem.size(), format.size));
            } else {
                (void)
                    format; // Unreferenced if constexpr RepType != string_view
            }
            data_view<char> view(data, bigEndian);
            data::store(view, elem);
            return 0;
        }

        template <typename RepType, typename T>
        static constexpr auto convert(const T &val) -> RepType {
            // If T is char[], and RepType is string_view - construct directly
            // with std::size(val)
            //  because std::string_view doesn't have a constructor taking a
            //  char(&)[]
            if constexpr (std::is_array_v<T>
                          && std::is_same_v<std::remove_extent_t<T>, char>
                          && std::is_same_v<RepType, std::string_view>) {
                return RepType(std::data(val), std::size(val));
            } else {
                return static_cast<RepType>(val);
            }
        }

        template <size_t... Items, typename... Args>
        static auto pack(std::index_sequence<Items...> /*unused*/,
                         Args &&...args) {
            constexpr auto mode = format_mode();
            PRINT("format mode: {}", mode.format());

            using ArrayType = std::array<char, calcsize()>;

            auto                 output = ArrayType{};
            constexpr FormatType formats[] = {type_of_item<Items>()...};
            using Types = std::tuple<
                RepresentedType<decltype(mode), formats[Items].formatChar>...>;

            // Convert args to a tuple of the represented types
            Types types
                = std::make_tuple(convert<std::tuple_element_t<Items, Types>>(
                    std::forward<Args>(args))...);
            constexpr size_t offsets[] = {binary_offset<Items>()...};
            int              _[] = {pack_element(output.data() + offsets[Items],
                                    mode.is_big_endian(),
                                    formats[Items],
                                    std::get<Items>(types))...};
            (void) _;
            return output;
        }
    };

} // namespace detail

template <string_container container, typename... Args>
auto new_pack(Args &&...args) {
    using Fmt = detail::fmt_string<container>;
    constexpr size_t N = Fmt::count_items();
    static_assert(N == sizeof...(args), "Parameter number does not match");

    return Fmt::pack(std::make_index_sequence<N>{},
                     std::forward<Args>(args)...);
}
} // namespace struct_pack
