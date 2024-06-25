#include "struct_cpp/string_literal.hpp"

namespace struct_cpp {
template <string_container container>
auto new_pack() {
    return type_string<container>{};
}
} // namespace struct_cpp
