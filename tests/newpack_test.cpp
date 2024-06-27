#include "struct_pack.hpp"
#include "struct_pack/debug.hpp"

auto main() -> int {
    auto packed = struct_pack::new_pack<"!BB">(0x12, 0x34);
    PRINT("packed[0] = 0x{:02x}", packed[0]);
    PRINT("packed[1] = 0x{:02x}", packed[1]);
    ASSERT(packed[0] == 0x12);
    ASSERT(packed[1] == 0x34);

    // B: unsigned char      (1byte)
    // H: unsigned short     (2bytes)
    // I: unsigned int       (4bytes)
    // L: unsigned long      (4bytes)
    // Q: unsigned long long (8bytes)
    // \xfe\xff\xfe\xff\xff\xff\xfe\xff\xff\xff\xfe\xff\xff\xff\xff\xff\xff\xff\xfe
    auto packed2 = struct_pack::new_pack<">BHILQ">(254,
                                                   65534,
                                                   4294967294UL,
                                                   4294967294UL,
                                                   18446744073709551614ULL);
    PRINT("packed2[0] = 0x{:02X}", packed2[0]);
    ASSERT(packed2[0] == (char) 0xFE);

    PRINT("packed2[1] = 0x{:02X}", packed2[1]);
    PRINT("packed2[2] = 0x{:02X}", packed2[2]);
    ASSERT(packed2[1] == (char) 0xFF);
    ASSERT(packed2[2] == (char) 0xFE);

    PRINT("packed2[3] = 0x{:02X}", packed2[3]);
    PRINT("packed2[4] = 0x{:02X}", packed2[4]);
    PRINT("packed2[5] = 0x{:02X}", packed2[5]);
    PRINT("packed2[6] = 0x{:02X}", packed2[6]);
    ASSERT(packed2[3] == (char) 0xFF);
    ASSERT(packed2[4] == (char) 0xFF);
    ASSERT(packed2[5] == (char) 0xFF);
    ASSERT(packed2[6] == (char) 0xFE);

    PRINT("packed2[7] = 0x{:02X}", packed2[7]);
    PRINT("packed2[8] = 0x{:02X}", packed2[8]);
    PRINT("packed2[9] = 0x{:02X}", packed2[9]);
    PRINT("packed2[10] = 0x{:02X}", packed2[10]);
    ASSERT(packed2[7] == (char) 0xFF);
    ASSERT(packed2[8] == (char) 0xFF);
    ASSERT(packed2[9] == (char) 0xFF);
    ASSERT(packed2[10] == (char) 0xFE);

    PRINT("packed2[11] = 0x{:02X}", packed2[11]);
    PRINT("packed2[12] = 0x{:02X}", packed2[12]);
    PRINT("packed2[13] = 0x{:02X}", packed2[13]);
    PRINT("packed2[14] = 0x{:02X}", packed2[14]);
    PRINT("packed2[15] = 0x{:02X}", packed2[15]);
    PRINT("packed2[16] = 0x{:02X}", packed2[16]);
    PRINT("packed2[17] = 0x{:02X}", packed2[17]);
    PRINT("packed2[18] = 0x{:02X}", packed2[18]);
    ASSERT(packed2[11] == (char) 0xFF);
    ASSERT(packed2[12] == (char) 0xFF);
    ASSERT(packed2[13] == (char) 0xFF);
    ASSERT(packed2[14] == (char) 0xFF);
    ASSERT(packed2[15] == (char) 0xFF);
    ASSERT(packed2[16] == (char) 0xFF);
    ASSERT(packed2[17] == (char) 0xFF);
    ASSERT(packed2[18] == (char) 0xFE);
}
