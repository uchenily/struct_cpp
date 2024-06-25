#include "struct_cpp/string_literal.hpp"
#include <tuple>

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
        return ch == '<' || ch == '>' || ch == '!' || ch == '=' || ch == '@';
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
};

namespace struct_cpp {
template <string_container container, typename... Args>
auto new_pack(Args... args) {
    auto fmt = fmt_string<container>{};

    constexpr size_t item_count = fmt.count_items();
    static_assert(item_count == sizeof...(args));

    return fmt;
}
} // namespace struct_cpp
