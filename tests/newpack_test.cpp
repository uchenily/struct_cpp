#include "struct_pack.hpp"

auto main() -> int {
    struct_pack::new_pack<"!BB">(0x12, 0x34);
}
