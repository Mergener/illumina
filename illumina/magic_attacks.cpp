#include "attacks.h"

#ifndef USE_PEXT

namespace illumina {

Bitboard g_bishop_attacks[64][512];
Bitboard g_rook_attacks[64][4096];

const Bitboard g_bishop_masks[] {
    0x0040201008040200ULL, 0x0000402010080400ULL, 0x0000004020100a00ULL, 0x0000000040221400ULL,
    0x0000000002442800ULL, 0x0000000204085000ULL, 0x0000020408102000ULL, 0x0002040810204000ULL,
    0x0020100804020000ULL, 0x0040201008040000ULL, 0x00004020100a0000ULL, 0x0000004022140000ULL,
    0x0000000244280000ULL, 0x0000020408500000ULL, 0x0002040810200000ULL, 0x0004081020400000ULL,
    0x0010080402000200ULL, 0x0020100804000400ULL, 0x004020100a000a00ULL, 0x0000402214001400ULL,
    0x0000024428002800ULL, 0x0002040850005000ULL, 0x0004081020002000ULL, 0x0008102040004000ULL,
    0x0008040200020400ULL, 0x0010080400040800ULL, 0x0020100a000a1000ULL, 0x0040221400142200ULL,
    0x0002442800284400ULL, 0x0004085000500800ULL, 0x0008102000201000ULL, 0x0010204000402000ULL,
    0x0004020002040800ULL, 0x0008040004081000ULL, 0x00100a000a102000ULL, 0x0022140014224000ULL,
    0x0044280028440200ULL, 0x0008500050080400ULL, 0x0010200020100800ULL, 0x0020400040201000ULL,
    0x0002000204081000ULL, 0x0004000408102000ULL, 0x000a000a10204000ULL, 0x0014001422400000ULL,
    0x0028002844020000ULL, 0x0050005008040200ULL, 0x0020002010080400ULL, 0x0040004020100800ULL,
    0x0000020408102000ULL, 0x0000040810204000ULL, 0x00000a1020400000ULL, 0x0000142240000000ULL,
    0x0000284402000000ULL, 0x0000500804020000ULL, 0x0000201008040200ULL, 0x0000402010080400ULL,
    0x0002040810204000ULL, 0x0004081020400000ULL, 0x000a102040000000ULL, 0x0014224000000000ULL,
    0x0028440200000000ULL, 0x0050080402000000ULL, 0x0020100804020000ULL, 0x0040201008040200ULL
};

const Bitboard g_rook_masks[] {
    0x000101010101017eULL, 0x000202020202027cULL, 0x000404040404047aULL, 0x0008080808080876ULL,
    0x001010101010106eULL, 0x002020202020205eULL, 0x004040404040403eULL, 0x008080808080807eULL,
    0x0001010101017e00ULL, 0x0002020202027c00ULL, 0x0004040404047a00ULL, 0x0008080808087600ULL,
    0x0010101010106e00ULL, 0x0020202020205e00ULL, 0x0040404040403e00ULL, 0x0080808080807e00ULL,
    0x00010101017e0100ULL, 0x00020202027c0200ULL, 0x00040404047a0400ULL, 0x0008080808760800ULL,
    0x00101010106e1000ULL, 0x00202020205e2000ULL, 0x00404040403e4000ULL, 0x00808080807e8000ULL,
    0x000101017e010100ULL, 0x000202027c020200ULL, 0x000404047a040400ULL, 0x0008080876080800ULL,
    0x001010106e101000ULL, 0x002020205e202000ULL, 0x004040403e404000ULL, 0x008080807e808000ULL,
    0x0001017e01010100ULL, 0x0002027c02020200ULL, 0x0004047a04040400ULL, 0x0008087608080800ULL,
    0x0010106e10101000ULL, 0x0020205e20202000ULL, 0x0040403e40404000ULL, 0x0080807e80808000ULL,
    0x00017e0101010100ULL, 0x00027c0202020200ULL, 0x00047a0404040400ULL, 0x0008760808080800ULL,
    0x00106e1010101000ULL, 0x00205e2020202000ULL, 0x00403e4040404000ULL, 0x00807e8080808000ULL,
    0x007e010101010100ULL, 0x007c020202020200ULL, 0x007a040404040400ULL, 0x0076080808080800ULL,
    0x006e101010101000ULL, 0x005e202020202000ULL, 0x003e404040404000ULL, 0x007e808080808000ULL,
    0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL,
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL
};

const int g_bishop_shifts[] {
    58, 59, 59, 59, 59, 59, 59, 58,
    59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59,
    58, 59, 59, 59, 59, 59, 59, 58
};

const int g_rook_shifts[] {
    52, 53, 53, 53, 53, 53, 53, 52,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    52, 53, 53, 53, 53, 53, 53, 52
};

const Bitboard g_rook_magics[] {
    0x880005021864000ULL, 0x8240032000401008ULL, 0x200082040120080ULL, 0x100080421001000ULL,
    0x600040850202200ULL, 0x1080018004000200ULL, 0x2100008200044100ULL, 0x2980012100034080ULL,
    0x1b02002040810200ULL, 0x410401000402002ULL, 0x3003803000200080ULL, 0x1801001000090020ULL,
    0x442000408120020ULL, 0x800200800400ULL, 0xc804200010080ULL, 0x810100010000a042ULL,
    0x1218001804000ULL, 0x102a0a0020408100ULL, 0x6410020001100ULL, 0x800090020100100ULL,
    0xc301010008000411ULL, 0x800a010100040008ULL, 0x1080010100020004ULL, 0x8040020004810074ULL,
    0x200802080004000ULL, 0x1010024240002002ULL, 0x2048200102040ULL, 0x8121000900100022ULL,
    0x201011100080005ULL, 0x2c000480800200ULL, 0x4040101000200ULL, 0x8042008200040061ULL,
    0x10020c011800080ULL, 0x8040402000401000ULL, 0x200900082802000ULL, 0x11001001000822ULL,
    0x454c800800800400ULL, 0x4000800400800200ULL, 0x420458804000630ULL, 0x909000087000272ULL,
    0x380004020004000ULL, 0x110004020004013ULL, 0xa48104082020021ULL, 0x98048010008008ULL,
    0x20080004008080ULL, 0x202004490120028ULL, 0x1810288040010ULL, 0x1248004091020004ULL,
    0x900e082480450200ULL, 0x820008020400080ULL, 0x3820110020004100ULL, 0x439821000080080ULL,
    0x2000408201200ULL, 0x800400020080ULL, 0x8008900801020400ULL, 0xc810289047040200ULL,
    0x1401024080291202ULL, 0x104100208202ULL, 0x800401008200101ULL, 0x8a0500044210089ULL,
    0x6001510201892ULL, 0x2a82001021486402ULL, 0x4200a1081004ULL, 0x2040080402912ULL,
};

const Bitboard g_bishop_magics[] {
    0x4050041800440021ULL, 0x20040408445080ULL, 0xa906020a000020ULL, 0x4404440080610020ULL,
    0x2021091400000ULL, 0x900421000000ULL, 0x480210704204ULL, 0x120a42110101020ULL,
    0x200290020084ULL, 0x1140040400a2020cULL, 0x8000080811102000ULL, 0x404208a08a2ULL,
    0x2100084840840c10ULL, 0x1061110080140ULL, 0x1808210022000ULL, 0x8030842211042008ULL,
    0x8401020011400ULL, 0x10800810011040ULL, 0x1208500bb20020ULL, 0x98408404008880ULL,
    0xd2000c12020000ULL, 0x4200110082000ULL, 0x901200040c824800ULL, 0x100220c104050480ULL,
    0x200260000a200408ULL, 0x210a84090020680ULL, 0x800c040202002400ULL, 0x80190401080208a0ULL,
    0xc03a84008280a000ULL, 0x8040804100a001ULL, 0x8010010808880ULL, 0x2210020004a0810ULL,
    0x8041000414218ULL, 0x2842015004600200ULL, 0x2102008200900020ULL, 0x230a008020820201ULL,
    0xc080200252008ULL, 0x9032004500c21000ULL, 0x120a04010a2098ULL, 0x200848582010421ULL,
    0xb0021a10061440c6ULL, 0x4a0d0120100810ULL, 0x80010a4402101000ULL, 0x8810222018000100ULL,
    0x20081010101100ULL, 0x8081000200410ULL, 0x50a00800a1104080ULL, 0x10020441184842ULL,
    0x4811012110402000ULL, 0x12088088092a40ULL, 0x8120846480000ULL, 0x8800062880810ULL,
    0x4010802020412010ULL, 0xc10008503006200aULL, 0x144300202042711ULL, 0xa103441014440ULL,
    0x20804400c44001ULL, 0x100210882300208ULL, 0x8220200840413ULL, 0x1144800b841400ULL,
    0x4460010010202202ULL, 0x1000a10410202ULL, 0x1092200481020400ULL, 0x40420041c002047ULL,
};

static Bitboard generate_slider_attacks(Square s, Direction dir, Bitboard occ) {
    Bitboard ret = 0;

    while (true) {
        BoardFile prev_file = square_file(s);
        BoardRank prev_rank = square_rank(s);

        s += dir;

        int file_delta = std::abs(square_file(s) - prev_file);
        int rank_delta = std::abs(square_rank(s) - prev_rank);

        if ((dir == DIR_EAST || dir == DIR_WEST) &&
            rank_delta != 0) {
            // Rank moved.
            break;
        }

        if ((dir == DIR_NORTHWEST || dir == DIR_NORTHEAST ||
             dir == DIR_SOUTHEAST || dir == DIR_SOUTHWEST) && // diagonal
            (file_delta != 1 || rank_delta != 1)) {
            // Out of diagonal.
            break;
        }

        if (s < 0 || s >= 64) {
            // Out of bounds.
            break;
        }

        ret = set_bit(ret, s);

        if (bit_is_set(occ, s)) {
            break;
        }
    }

    return ret;
}

static Bitboard generate_bishop_attacks(Square s, Bitboard occ) {
    Bitboard ret = 0;

    ret |= generate_slider_attacks(s, DIR_NORTHEAST, occ);
    ret |= generate_slider_attacks(s, DIR_SOUTHEAST, occ);
    ret |= generate_slider_attacks(s, DIR_SOUTHWEST, occ);
    ret |= generate_slider_attacks(s, DIR_NORTHWEST, occ);

    return ret;
}

static Bitboard generate_rook_attacks(Square s, Bitboard occ) {
    Bitboard ret = 0;

    ret |= generate_slider_attacks(s, DIR_NORTH, occ);
    ret |= generate_slider_attacks(s, DIR_SOUTH, occ);
    ret |= generate_slider_attacks(s, DIR_EAST, occ);
    ret |= generate_slider_attacks(s, DIR_WEST, occ);

    return ret;
}


static Bitboard generate_occupancy(Bitboard mask, ui64 index) {
    Bitboard ret = 0;
    Square it = 0;
    Bitboard idx_bb = index;

    while (mask != 0) {
        Square s = lsb(mask);

        if (bit_is_set(idx_bb, it)) {
            ret = set_bit(ret, s);
        }

        mask = unset_lsb(mask);
        it++;
    }

    return ret;
}

static void generate_slider_bitboards() {
    for (Square s = 0; s < 64; ++s) {
        // Generate bishop attacks.
        int bishop_shift = g_bishop_shifts[s];
        ui64 bishop_entries = ((1ULL) << (64 - bishop_shift));

        for (ui64 i = 0; i < bishop_entries; ++i) {
            Bitboard occ = generate_occupancy(g_bishop_masks[s], i);
            ui64 key = (occ * g_bishop_magics[s]) >> bishop_shift;
            g_bishop_attacks[s][key] = generate_bishop_attacks(s, occ);
        }

        // Generate rook attacks.
        int rook_shift = g_rook_shifts[s];
        ui64 rook_entries = ((1ULL) << (64 - rook_shift));

        for (ui64 i = 0; i < rook_entries; ++i) {
            Bitboard occ = generate_occupancy(g_rook_masks[s], i);
            ui64 key = (occ * g_rook_magics[s]) >> rook_shift;
            g_rook_attacks[s][key] = generate_rook_attacks(s, occ);
        }
    }
}

void init_magic_bbs() {
    generate_slider_bitboards();
}

} // illumina

#endif // USE_PEXT