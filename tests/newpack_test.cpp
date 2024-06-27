#include "struct_pack.hpp"
#include "struct_pack/debug.hpp"

auto main() -> int {
    auto packed = struct_pack::new_pack<"!BB">(0x12, 0x34);
    PRINT("packed[0] = 0x{:02x}", packed[0]);
    PRINT("packed[1] = 0x{:02x}", packed[1]);
    ASSERT(packed[0] == 0x12);
    ASSERT(packed[1] == 0x34);
}
