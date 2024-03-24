#include "../litetest/litetest.h"

#include <iostream>

#include "types.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(Types);

TEST_CASE(MsbLsb) {
    struct {
        ui64 n;
        ui8  expected_lsb;
        ui8  expected_msb;

        void run() {
            EXPECT(lsb(n)).to_be(expected_lsb);
            EXPECT(msb(n)).to_be(expected_msb);
        }
    } tests[] = {
        { 0x124a79acULL, 2, 28 },
        { 0x8d638400ULL, 10, 31 },
        { 0x10000000ULL, 28, 28 },
        { 0x5c763000ULL, 12, 30 },
        { 0xf91d1000ULL, 12, 31 },
        { 0x50c98000ULL, 15, 30 },
        { 0xc0800000ULL, 23, 31 },
        { 0x96b8000ULL, 15, 27 },
        { 0x15cc0900ULL, 8, 28 },
        { 0x9a623780ULL, 7, 31 },
        { 0x66b0000ULL, 16, 26 },
        { 0xe8f54428ULL, 3, 31 },
        { 0x1040000ULL, 18, 24 },
        { 0xd0580000ULL, 19, 31 },
        { 0x8bdcc000ULL, 14, 31 },
        { 0x92600000ULL, 21, 31 },
        { 0x39200000ULL, 21, 29 },
        { 0x58300000ULL, 20, 30 },
        { 0xb5f541d0ULL, 4, 31 },
        { 0xbc000000ULL, 26, 31 },
        { 0x7c800000ULL, 23, 30 },
        { 0x882b5000ULL, 12, 31 },
        { 0x52cac000ULL, 14, 30 },
        { 0x40000000ULL, 30, 30 },
        { 0xc1f15b06ULL, 1, 31 },
        { 0xe1741200ULL, 9, 31 },
        { 0xb8000000ULL, 27, 31 },
        { 0xd7000000ULL, 24, 31 },
        { 0xe2db3664ULL, 2, 31 },
        { 0x30000000ULL, 28, 29 },
        { 0x11740000ULL, 18, 28 },
        { 0xd46fbe50ULL, 4, 31 },
        { 0x1f42f01aULL, 1, 28 },
        { 0xf4418000ULL, 15, 31 },
        { 0x6a111400ULL, 10, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0xe4000000ULL, 26, 31 },
        { 0xffc00000ULL, 22, 31 },
        { 0x33000000ULL, 24, 29 },
        { 0x60faf000ULL, 12, 30 },
        { 0xf0000000ULL, 28, 31 },
        { 0x20000000ULL, 29, 29 },
        { 0xaae52340ULL, 6, 31 },
        { 0xf64e01c8ULL, 3, 31 },
        { 0x1b7778a0ULL, 5, 28 },
        { 0x64bd800ULL, 11, 26 },
        { 0x2d7c000ULL, 14, 25 },
        { 0x76beb80ULL, 7, 26 },
        { 0xb5aa8f70ULL, 4, 31 },
        { 0x81400000ULL, 22, 31 },
        { 0xefd118b8ULL, 3, 31 },
        { 0x73400000ULL, 22, 30 },
        { 0x7d480000ULL, 19, 30 },
        { 0x4c7c2700ULL, 8, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0xe0000000ULL, 29, 31 },
        { 0x560f0000ULL, 16, 30 },
        { 0xd0000000ULL, 28, 31 },
        { 0x51500000ULL, 20, 30 },
        { 0x40000000ULL, 30, 30 },
        { 0x74000000ULL, 26, 30 },
        { 0xec270000ULL, 16, 31 },
        { 0x32ad9880ULL, 7, 29 },
        { 0x80000000ULL, 31, 31 },
        { 0xa7868000ULL, 15, 31 },
        { 0xfebd0000ULL, 16, 31 },
        { 0xb33d58c0ULL, 6, 31 },
        { 0x59b42cf0ULL, 4, 30 },
        { 0x9db826c0ULL, 6, 31 },
        { 0x9810bcc0ULL, 6, 31 },
        { 0x70000000ULL, 28, 30 },
        { 0xc8a7b556ULL, 1, 31 },
        { 0xf2800000ULL, 23, 31 },
        { 0x53800000ULL, 23, 30 },
        { 0xbc31dc00ULL, 10, 31 },
        { 0xb067d400ULL, 10, 31 },
        { 0x7476ba94ULL, 2, 30 },
        { 0xd9dd2400ULL, 10, 31 },
        { 0xd0ac8700ULL, 8, 31 },
        { 0x9c535b00ULL, 8, 31 },
        { 0x340f6000ULL, 13, 29 },
        { 0x26000000ULL, 25, 29 },
        { 0x4d9d4000ULL, 14, 30 },
        { 0x62400000ULL, 22, 30 },
        { 0x399b8be0ULL, 5, 29 },
        { 0xa0000000ULL, 29, 31 },
        { 0x5c0b3000ULL, 12, 30 },
        { 0xf48e8000ULL, 15, 31 },
        { 0x60744eb0ULL, 4, 30 },
        { 0xa8850000ULL, 16, 31 },
        { 0x40e00000ULL, 21, 30 },
        { 0x9d3c0448ULL, 3, 31 },
        { 0xb8000000ULL, 27, 31 },
        { 0x468507feULL, 1, 30 },
        { 0x2a898000ULL, 15, 29 },
        { 0x9c00000ULL, 22, 27 },
        { 0xe851df7aULL, 1, 31 },
        { 0x20000000ULL, 29, 29 },
        { 0xd89ac000ULL, 14, 31 },
        { 0x12763fb8ULL, 3, 28 },
        { 0x56000000ULL, 25, 30 },
        { 0x83929800ULL, 11, 31 },
        { 0xc6000000ULL, 25, 31 },
        { 0x58000000ULL, 27, 30 },
        { 0x69d00000ULL, 20, 30 },
        { 0xa7a00000ULL, 21, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0x92000000ULL, 25, 31 },
        { 0xefd81e08ULL, 3, 31 },
        { 0xe6600000ULL, 21, 31 },
        { 0x99679878ULL, 3, 31 },
        { 0x70c00000ULL, 22, 30 },
        { 0xc0000000ULL, 30, 31 },
        { 0x28e80000ULL, 19, 29 },
        { 0x3c830000ULL, 16, 29 },
        { 0xcb110000ULL, 16, 31 },
        { 0x35980000ULL, 19, 29 },
        { 0xa5800000ULL, 23, 31 },
        { 0x29b28000ULL, 15, 29 },
        { 0x40000000ULL, 30, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0x8ec00000ULL, 22, 31 },
        { 0x48000000ULL, 27, 30 },
        { 0xec600000ULL, 21, 31 },
        { 0x6c040400ULL, 10, 30 },
        { 0xa460f67dULL, 0, 31 },
        { 0xdbc00000ULL, 22, 31 },
        { 0x48000000ULL, 27, 30 },
        { 0xa5500000ULL, 20, 31 },
        { 0xa5180000ULL, 19, 31 },
        { 0x34000000ULL, 26, 29 },
        { 0x75880000ULL, 19, 30 },
        { 0xd8600000ULL, 21, 31 },
        { 0xa414c490ULL, 4, 31 },
        { 0xaf000000ULL, 24, 31 },
        { 0x430000ULL, 16, 22 },
        { 0xd0bc4000ULL, 14, 31 },
        { 0xd625b330ULL, 4, 31 },
        { 0x60000000ULL, 29, 30 },
        { 0xbed5ea00ULL, 9, 31 },
        { 0xd6fccb00ULL, 8, 31 },
        { 0x4f4f4400ULL, 10, 30 },
        { 0x4d7c6400ULL, 10, 30 },
        { 0x7f000000ULL, 24, 30 },
        { 0x40000000ULL, 30, 30 },
        { 0xe876000ULL, 13, 27 },
        { 0xed000000ULL, 24, 31 },
        { 0x94b60000ULL, 17, 31 },
        { 0xa8e3b400ULL, 10, 31 },
        { 0xb52213e8ULL, 3, 31 },
        { 0xe79c3000ULL, 12, 31 },
        { 0x86100000ULL, 20, 31 },
        { 0x2d87eac0ULL, 6, 29 },
        { 0xbea4f35aULL, 1, 31 },
        { 0xf4287ef4ULL, 2, 31 },
        { 0x50000000ULL, 28, 30 },
        { 0x51600000ULL, 21, 30 },
        { 0x7f140000ULL, 18, 30 },
        { 0x3b81c800ULL, 11, 29 },
        { 0x5ae00000ULL, 21, 30 },
        { 0xe0000000ULL, 29, 31 },
        { 0x24d79880ULL, 7, 29 },
        { 0xd2d4f365ULL, 0, 31 },
        { 0x5715c000ULL, 14, 30 },
        { 0x4285f588ULL, 3, 30 },
        { 0x35a00000ULL, 21, 29 },
        { 0xade4fe28ULL, 3, 31 },
        { 0x80000000ULL, 31, 31 },
        { 0x72d351feULL, 1, 30 },
        { 0x4f7c0490ULL, 4, 30 },
        { 0x67dc0000ULL, 18, 30 },
        { 0x1c000000ULL, 26, 28 },
        { 0x93b15000ULL, 12, 31 },
        { 0x9b04b41bULL, 0, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xf09a0000ULL, 17, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0x7d60000ULL, 17, 26 },
        { 0xfe680000ULL, 19, 31 },
        { 0x18c00000ULL, 22, 28 },
        { 0x702ca000ULL, 13, 30 },
        { 0x40000000ULL, 30, 30 },
        { 0x86d36b0cULL, 2, 31 },
        { 0x38f18000ULL, 15, 29 },
        { 0x13680000ULL, 19, 28 },
        { 0x8a000000ULL, 25, 31 },
        { 0xd6b60000ULL, 17, 31 },
        { 0x6ab40000ULL, 18, 30 },
        { 0xc1800000ULL, 23, 31 },
        { 0x80000000ULL, 31, 31 },
        { 0x69ec5600ULL, 9, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0x21582000ULL, 13, 29 },
        { 0x2848c800ULL, 11, 29 },
        { 0xabe40000ULL, 18, 31 },
        { 0xfc323400ULL, 10, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xc46f1800ULL, 11, 31 },
        { 0xa8000000ULL, 27, 31 },
        { 0x7ee79879ULL, 0, 30 },
        { 0x67d00000ULL, 20, 30 },
        { 0xaf2fcc00ULL, 10, 31 },
        { 0x51045b7cULL, 2, 30 },
        { 0x6ecdec1cULL, 2, 30 },
        { 0x43a5dd28ULL, 3, 30 },
        { 0x84000000ULL, 26, 31 },
        { 0xa3ac0000ULL, 18, 31 },
        { 0x24614500ULL, 8, 29 },
        { 0x95260000ULL, 17, 31 },
        { 0xe0000000ULL, 29, 31 },
        { 0xe6e56000ULL, 13, 31 },
        { 0xbda43a20ULL, 5, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xa1ad400ULL, 10, 27 },
        { 0x3c500400ULL, 10, 29 },
        { 0x914c9b30ULL, 4, 31 },
        { 0x7027a600ULL, 9, 30 },
        { 0x990aada0ULL, 5, 31 },
        { 0x80000000ULL, 31, 31 },
        { 0xbcb8d542ULL, 1, 31 },
        { 0xe33a0000ULL, 17, 31 },
        { 0xa8000000ULL, 27, 31 },
        { 0x8e000000ULL, 25, 31 },
        { 0x60000000ULL, 29, 30 },
        { 0xa4a0000ULL, 17, 27 },
        { 0xfdf68000ULL, 15, 31 },
        { 0x50759580ULL, 7, 30 },
        { 0x42790000ULL, 16, 30 },
        { 0x8fe9e000ULL, 13, 31 },
        { 0x7400000ULL, 22, 26 },
        { 0xe06b6070ULL, 4, 31 },
        { 0xa1240000ULL, 18, 31 },
        { 0xdb6b9f52ULL, 1, 31 },
        { 0x3dc00000ULL, 22, 29 },
        { 0x664093c0ULL, 6, 30 },
        { 0x40000000ULL, 30, 30 },
        { 0x2d800000ULL, 23, 29 },
        { 0x5c000000ULL, 26, 30 },
        { 0x7e900000ULL, 20, 30 },
        { 0x68a09000ULL, 12, 30 },
        { 0xf7e00000ULL, 21, 31 },
        { 0x2e02fc10ULL, 4, 29 },
        { 0xee000000ULL, 25, 31 },
        { 0x70780000ULL, 19, 30 },
        { 0x33000000ULL, 24, 29 },
        { 0xdc800000ULL, 23, 31 },
        { 0x52ea7f80ULL, 7, 30 },
        { 0xcd6be800ULL, 11, 31 },
        { 0x82c60600ULL, 9, 31 },
        { 0xa0000000ULL, 29, 31 },
        { 0x955a5600ULL, 9, 31 },
        { 0x1ead46c0ULL, 6, 28 },
        { 0xb78f0dc0ULL, 6, 31 },
        { 0x90000000ULL, 28, 31 },
        { 0xf6fcc400ULL, 10, 31 },
        { 0xf9b00000ULL, 20, 31 },
        { 0x80000000ULL, 31, 31 },
        { 0x53030000ULL, 16, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0xf05c8000ULL, 15, 31 },
        { 0x8e71a800ULL, 11, 31 },
        { 0x75be0000ULL, 17, 30 },
        { 0xb0000000ULL, 28, 31 },
        { 0x7d4ae000ULL, 13, 30 },
        { 0xb0f00000ULL, 20, 31 },
        { 0xa8190400ULL, 10, 31 },
        { 0x56cdd800ULL, 11, 30 },
        { 0x68000000ULL, 27, 30 },
        { 0xd1380000ULL, 19, 31 },
        { 0x49900000ULL, 20, 30 },
        { 0x4000000ULL, 26, 26 },
        { 0x40794000ULL, 14, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0x114fb100ULL, 8, 28 },
        { 0x9558a800ULL, 11, 31 },
        { 0x68000000ULL, 27, 30 },
        { 0x58f80000ULL, 19, 30 },
        { 0x94000000ULL, 26, 31 },
        { 0xd2000000ULL, 25, 31 },
        { 0x84000000ULL, 26, 31 },
        { 0x16800000ULL, 23, 28 },
        { 0xa0000000ULL, 29, 31 },
        { 0x6d0d1800ULL, 11, 30 },
        { 0xe184ae3aULL, 1, 31 },
        { 0x67c33000ULL, 12, 30 },
        { 0xda648580ULL, 7, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xa3e00000ULL, 21, 31 },
        { 0xcc000000ULL, 26, 31 },
        { 0x78000000ULL, 27, 30 },
        { 0x73989800ULL, 11, 30 },
        { 0x34600000ULL, 21, 29 },
        { 0xae982000ULL, 13, 31 },
        { 0x4aec0000ULL, 18, 30 },
        { 0x6b4a0000ULL, 17, 30 },
        { 0x3a000000ULL, 25, 29 },
        { 0xd3980000ULL, 19, 31 },
        { 0xa38f11e0ULL, 5, 31 },
        { 0x58000000ULL, 27, 30 },
        { 0x5e4e7e00ULL, 9, 30 },
        { 0x8ea7f7a0ULL, 5, 31 },
        { 0x6fc7c168ULL, 3, 30 },
        { 0xee6b3680ULL, 7, 31 },
        { 0x6bd20000ULL, 17, 30 },
        { 0x9b7464c0ULL, 6, 31 },
        { 0x5afe0400ULL, 10, 30 },
        { 0x25270b02ULL, 1, 29 },
        { 0xa6d933daULL, 1, 31 },
        { 0xb121a800ULL, 11, 31 },
        { 0x1d205a88ULL, 3, 28 },
        { 0xc0000000ULL, 30, 31 },
        { 0xc2ec4f0eULL, 1, 31 },
        { 0xcb280000ULL, 19, 31 },
        { 0xe0000000ULL, 29, 31 },
        { 0xea17f800ULL, 11, 31 },
        { 0xa1a364a4ULL, 2, 31 },
        { 0xa3dd02b3ULL, 0, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0x8a600000ULL, 21, 31 },
        { 0x5cf47a00ULL, 9, 30 },
        { 0xad2b8bf2ULL, 1, 31 },
        { 0x27d03000ULL, 12, 29 },
        { 0x80000000ULL, 31, 31 },
        { 0x450a0000ULL, 17, 30 },
        { 0x946f0000ULL, 16, 31 },
        { 0x46c00000ULL, 22, 30 },
        { 0xedf00000ULL, 20, 31 },
        { 0xb4333c00ULL, 10, 31 },
        { 0x70f33000ULL, 12, 30 },
        { 0x42b00000ULL, 20, 30 },
        { 0xaa680000ULL, 19, 31 },
        { 0xe7000000ULL, 24, 31 },
        { 0x9e6c0000ULL, 18, 31 },
        { 0x60000000ULL, 29, 30 },
        { 0xa000000ULL, 25, 27 },
        { 0xb4000000ULL, 26, 31 },
        { 0x7d615400ULL, 10, 30 },
        { 0x80000000ULL, 31, 31 },
        { 0x24000000ULL, 26, 29 },
        { 0x43640000ULL, 18, 30 },
        { 0xbb000000ULL, 24, 31 },
        { 0x20000000ULL, 29, 29 },
        { 0x89fa7238ULL, 3, 31 },
        { 0x89389300ULL, 8, 31 },
        { 0x9ffba377ULL, 0, 31 },
        { 0x47c0fdc0ULL, 6, 30 },
        { 0x26a27b80ULL, 7, 29 },
        { 0x512b2000ULL, 13, 30 },
        { 0xc1800000ULL, 23, 31 },
        { 0xc0000000ULL, 30, 31 },
        { 0xed000000ULL, 24, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0x98000000ULL, 27, 31 },
        { 0xb0eac000ULL, 14, 31 },
        { 0xbd05de40ULL, 6, 31 },
        { 0x9ff8c000ULL, 14, 31 },
        { 0x59ac0000ULL, 18, 30 },
        { 0x14000000ULL, 26, 28 },
        { 0xd4146406ULL, 1, 31 },
        { 0xa1000000ULL, 24, 31 },
        { 0x90ca0000ULL, 17, 31 },
        { 0x50800000ULL, 23, 30 },
        { 0x7857ac68ULL, 3, 30 },
        { 0x86be1360ULL, 5, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xff9b1fe8ULL, 3, 31 },
        { 0xb0000000ULL, 28, 31 },
        { 0x40000000ULL, 30, 30 },
        { 0xf7510000ULL, 16, 31 },
        { 0x64000000ULL, 26, 30 },
        { 0x90000000ULL, 28, 31 },
        { 0xe800000ULL, 23, 27 },
        { 0xa384ce00ULL, 9, 31 },
        { 0x472e8700ULL, 8, 30 },
        { 0x60561240ULL, 6, 30 },
        { 0x23a00000ULL, 21, 29 },
        { 0x90000000ULL, 28, 31 },
        { 0x98000000ULL, 27, 31 },
        { 0xd8000000ULL, 27, 31 },
        { 0x50000000ULL, 28, 30 },
        { 0x2b973d8ULL, 3, 25 },
        { 0xc844b218ULL, 3, 31 },
        { 0x93170000ULL, 16, 31 },
        { 0x5ce75cd0ULL, 4, 30 },
        { 0x7f6916a0ULL, 5, 30 },
        { 0x4a000000ULL, 25, 30 },
        { 0x46e80000ULL, 19, 30 },
        { 0xc5739800ULL, 11, 31 },
        { 0x8d2d8000ULL, 15, 31 },
        { 0xb8b07400ULL, 10, 31 },
        { 0x1f604000ULL, 14, 28 },
        { 0xb0000000ULL, 28, 31 },
    };

    for (auto& t: tests) {
        t.run();
    }
}

TEST_CASE(OppositeColor) {
    EXPECT(opposite_color(CL_WHITE)).to_be(CL_BLACK);
    EXPECT(opposite_color(CL_BLACK)).to_be(CL_WHITE);
}

TEST_CASE(SquaresAndDirections) {
    EXPECT(SQ_F3 + DIR_NORTH).to_be(SQ_F4);
    EXPECT(SQ_F3 + DIR_SOUTH).to_be(SQ_F2);
    EXPECT(SQ_F3 + DIR_EAST).to_be(SQ_G3);
    EXPECT(SQ_F3 + DIR_WEST).to_be(SQ_E3);
    EXPECT(SQ_F3 + DIR_NORTHEAST).to_be(SQ_G4);
    EXPECT(SQ_F3 + DIR_NORTHWEST).to_be(SQ_E4);
    EXPECT(SQ_F3 + DIR_SOUTHEAST).to_be(SQ_G2);
    EXPECT(SQ_F3 + DIR_SOUTHWEST).to_be(SQ_E2);

    EXPECT(SQ_F3 - DIR_NORTH).to_be(SQ_F2);
    EXPECT(SQ_F3 - DIR_SOUTH).to_be(SQ_F4);
    EXPECT(SQ_F3 - DIR_EAST).to_be(SQ_E3);
    EXPECT(SQ_F3 - DIR_WEST).to_be(SQ_G3);
    EXPECT(SQ_F3 - DIR_NORTHEAST).to_be(SQ_E2);
    EXPECT(SQ_F3 - DIR_NORTHWEST).to_be(SQ_G2);
    EXPECT(SQ_F3 - DIR_SOUTHEAST).to_be(SQ_E4);
    EXPECT(SQ_F3 - DIR_SOUTHWEST).to_be(SQ_G4);
}

TEST_CASE(MirrorHorizontal) {
    EXPECT(mirror_horizontal(SQ_H1)).to_be(SQ_A1);
    EXPECT(mirror_horizontal(SQ_E5)).to_be(SQ_D5);
}

TEST_CASE(MirrorVertical) {
    EXPECT(mirror_vertical(SQ_H1)).to_be(SQ_H8);
    EXPECT(mirror_vertical(SQ_E5)).to_be(SQ_E4);
}

TEST_CASE(ChebyshevDistance) {
    // Adjacent squares
    EXPECT(chebyshev_distance(SQ_F3, SQ_F4)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_F2)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_G3)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_E3)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_G4)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_E4)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_G2)).to_be(1);
    EXPECT(chebyshev_distance(SQ_F3, SQ_E2)).to_be(1);

    // Non-adjacent squares
    EXPECT(chebyshev_distance(SQ_F3, SQ_F7)).to_be(4);
    EXPECT(chebyshev_distance(SQ_F3, SQ_B3)).to_be(4);
    EXPECT(chebyshev_distance(SQ_F3, SQ_B7)).to_be(4);
}

TEST_CASE(ManhattanDistance) {
    // Adjacent squares
    EXPECT(manhattan_distance(SQ_F3, SQ_F4)).to_be(1);
    EXPECT(manhattan_distance(SQ_F3, SQ_F2)).to_be(1);
    EXPECT(manhattan_distance(SQ_F3, SQ_G3)).to_be(1);
    EXPECT(manhattan_distance(SQ_F3, SQ_E3)).to_be(1);
    EXPECT(manhattan_distance(SQ_F3, SQ_G4)).to_be(2);
    EXPECT(manhattan_distance(SQ_F3, SQ_E4)).to_be(2);
    EXPECT(manhattan_distance(SQ_F3, SQ_G2)).to_be(2);
    EXPECT(manhattan_distance(SQ_F3, SQ_E2)).to_be(2);

    // Non-adjacent squares
    EXPECT(manhattan_distance(SQ_F3, SQ_F7)).to_be(4);
    EXPECT(manhattan_distance(SQ_F3, SQ_B3)).to_be(4);
    EXPECT(manhattan_distance(SQ_F3, SQ_B7)).to_be(8);
}

TEST_CASE(Piece) {
    struct {
        Piece piece;
        PieceType type;
        Color color;
        char identifier;

        void test() const {
            EXPECT(ui32(piece.type())).to_be(ui32(type));
            EXPECT(ui32(piece.color())).to_be(ui32(color));
            EXPECT(piece.to_char()).to_be(identifier);
            EXPECT(Piece::from_char(identifier)).to_be(piece);
            EXPECT(Piece(piece.raw())).to_be(piece);
        }

    } cases[] = {
        { WHITE_PAWN,   PT_PAWN,   CL_WHITE, 'P' },
        { WHITE_KNIGHT, PT_KNIGHT, CL_WHITE, 'N' },
        { WHITE_BISHOP, PT_BISHOP, CL_WHITE, 'B' },
        { WHITE_ROOK,   PT_ROOK,   CL_WHITE, 'R' },
        { WHITE_QUEEN,  PT_QUEEN,  CL_WHITE, 'Q' },
        { WHITE_KING,   PT_KING,   CL_WHITE, 'K' },
        { BLACK_PAWN,   PT_PAWN,   CL_BLACK, 'p' },
        { BLACK_KNIGHT, PT_KNIGHT, CL_BLACK, 'n' },
        { BLACK_BISHOP, PT_BISHOP, CL_BLACK, 'b' },
        { BLACK_ROOK,   PT_ROOK,   CL_BLACK, 'r' },
        { BLACK_QUEEN,  PT_QUEEN,  CL_BLACK, 'q' },
        { BLACK_KING,   PT_KING,   CL_BLACK, 'k' },
    };

    for (const auto& test_case: cases) {
        test_case.test();
    }
}

TEST_CASE(ParseSquare) {
    // Test valid square strings
    EXPECT(parse_square("a1")).to_be(SQ_A1);
    EXPECT(parse_square("h8")).to_be(SQ_H8);
    EXPECT(parse_square("d4")).to_be(SQ_D4);
    EXPECT(parse_square("f6")).to_be(SQ_F6);
    EXPECT(parse_square("c3")).to_be(SQ_C3);

    EXPECT(parse_square("A1")).to_be(SQ_A1);
    EXPECT(parse_square("H8")).to_be(SQ_H8);
    EXPECT(parse_square("D4")).to_be(SQ_D4);
    EXPECT(parse_square("F6")).to_be(SQ_F6);
    EXPECT(parse_square("C3")).to_be(SQ_C3);
}

TEST_CASE(SquareName) {
    // Test square names
    EXPECT(square_name(SQ_A1)).to_be("a1");
    EXPECT(square_name(SQ_H8)).to_be("h8");
    EXPECT(square_name(SQ_D4)).to_be("d4");
    EXPECT(square_name(SQ_F6)).to_be("f6");
    EXPECT(square_name(SQ_C3)).to_be("c3");
}

TEST_CASE(MoveConstruction) {
    // Test constructing a normal move
    Move normal_move = Move::new_normal(SQ_E2, SQ_E4, Piece(CL_WHITE, PT_PAWN));
    EXPECT(normal_move.source()).to_be(SQ_E2);
    EXPECT(normal_move.destination()).to_be(SQ_E4);
    EXPECT(normal_move.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(normal_move.type()).to_be(MT_NORMAL);
    EXPECT(normal_move.to_uci()).to_be("e2e4");

    // Test constructing a simple capture move
    Move capture = Move::new_simple_capture(SQ_D4, SQ_E5, Piece(CL_WHITE, PT_PAWN), Piece(CL_BLACK, PT_KNIGHT));
    EXPECT(capture.source()).to_be(SQ_D4);
    EXPECT(capture.destination()).to_be(SQ_E5);
    EXPECT(capture.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(capture.captured_piece()).to_be(Piece(CL_BLACK, PT_KNIGHT));
    EXPECT(capture.type()).to_be(MT_SIMPLE_CAPTURE);
    EXPECT(capture.to_uci()).to_be("d4e5");

    // Test constructing a promotion cap ture move
    Move promotion_capture = Move::new_promotion_capture(SQ_G7, SQ_H8, CL_BLACK, Piece(CL_WHITE, PT_PAWN), PT_QUEEN);
    EXPECT(promotion_capture.source()).to_be(SQ_G7);
    EXPECT(promotion_capture.destination()).to_be(SQ_H8);
    EXPECT(promotion_capture.source_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(promotion_capture.captured_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(promotion_capture.promotion_piece_type()).to_be(PT_QUEEN);
    EXPECT(promotion_capture.type()).to_be(MT_PROMOTION_CAPTURE);
    EXPECT(promotion_capture.to_uci()).to_be("g7h8q");

    // Test constructing an en passant capture move
    Move en_passant = Move::new_en_passant_capture(SQ_D5, SQ_E6, CL_WHITE);
    EXPECT(en_passant.source()).to_be(SQ_D5);
    EXPECT(en_passant.destination()).to_be(SQ_E6);
    EXPECT(en_passant.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(en_passant.captured_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(en_passant.type()).to_be(MT_EN_PASSANT);
    EXPECT(en_passant.to_uci()).to_be("d5e6");

    // Test constructing a double push move
    Move double_push = Move::new_double_push(SQ_G2, CL_WHITE);
    EXPECT(double_push.source()).to_be(SQ_G2);
    EXPECT(double_push.destination()).to_be(SQ_G4);
    EXPECT(double_push.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(double_push.type()).to_be(MT_DOUBLE_PUSH);
    EXPECT(double_push.to_uci()).to_be("g2g4");

    // Test constructing a castling move
    Move castles = Move::new_castles(SQ_E1, CL_WHITE, SIDE_KING, SQ_H1);
    EXPECT(castles.source()).to_be(SQ_E1);
    EXPECT(castles.destination()).to_be(SQ_G1);
    EXPECT(castles.source_piece()).to_be(Piece(CL_WHITE, PT_KING));
    EXPECT(castles.castles_rook_src_square()).to_be(SQ_H1);
    EXPECT(castles.type()).to_be(MT_CASTLES);
    EXPECT(castles.to_uci()).to_be("e1g1");

    // Test constructing a simple promotion move
    Move promotion = Move::new_simple_promotion(SQ_H7, SQ_H8, CL_BLACK, PT_QUEEN);
    EXPECT(promotion.source()).to_be(SQ_H7);
    EXPECT(promotion.destination()).to_be(SQ_H8);
    EXPECT(promotion.source_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(promotion.promotion_piece_type()).to_be(PT_QUEEN);
    EXPECT(promotion.type()).to_be(MT_SIMPLE_PROMOTION);
    EXPECT(promotion.to_uci()).to_be("h7h8q");
}

TEST_CASE(ToUCI) {
    // Test normal move
    Move normal_move = Move::new_normal(SQ_E2, SQ_E4, Piece(CL_WHITE, PT_PAWN));
    EXPECT(normal_move.to_uci()).to_be("e2e4");

    // Test simple capture move
    Move capture = Move::new_simple_capture(SQ_D4, SQ_E5, Piece(CL_WHITE, PT_PAWN), Piece(CL_BLACK, PT_KNIGHT));
    EXPECT(capture.to_uci()).to_be("d4e5");

    // Test promotion capture move
    Move promotion_capture = Move::new_promotion_capture(SQ_G7, SQ_H8, CL_BLACK, Piece(CL_WHITE, PT_PAWN), PT_QUEEN);
    EXPECT(promotion_capture.to_uci()).to_be("g7h8q");

    // Test en passant capture move
    Move en_passant = Move::new_en_passant_capture(SQ_D5, SQ_E6, CL_WHITE);
    EXPECT(en_passant.to_uci()).to_be("d5e6");

    // Test double push move
    Move double_push = Move::new_double_push(SQ_G2, CL_WHITE);
    EXPECT(double_push.to_uci()).to_be("g2g4");

    // Test castling move
    Move castles = Move::new_castles(CL_WHITE, SIDE_KING);
    EXPECT(castles.to_uci()).to_be("e1g1");

    // Test simple promotion move
    Move promotion = Move::new_simple_promotion(SQ_H7, SQ_H8, CL_BLACK, PT_QUEEN);
    EXPECT(promotion.to_uci()).to_be("h7h8q");
}