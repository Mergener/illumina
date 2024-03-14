#include "../litetest/litetest.h"

#include "attacks.h"
#include "types.h"

using namespace litetest;
using namespace illumina;

// TODO: Many more test cases.

TEST_SUITE(Attacks);

TEST_CASE(PawnPushesAttacksCaptures) {
    struct {
        Square   square;
        Color    color;
        Bitboard occ;
        Bitboard expected_pushes;
        Bitboard expected_captures;

        void run() {
            EXPECT(pawn_pushes(square, color, occ)).to_be(expected_pushes);
            EXPECT(pawn_captures(square, color, occ)).to_be(expected_captures);
        }
    } tests[] = {
        { SQ_A2, CL_WHITE, 0, 0x1010000ULL, 0 },
        { SQ_A2, CL_WHITE, 0x20000ULL, 0x1010000ULL, 0x20000ULL },
        { SQ_A2, CL_WHITE, 0x820000ULL, 0x1010000ULL, 0x20000ULL },
        { SQ_A2, CL_WHITE, 0x100000000ULL, 0x1010000ULL, 0 },
        { SQ_A2, CL_WHITE, 0x1000000, 0x10000ULL, 0 },
    };

    for (auto& t: tests) {
        t.run();
    }
}

TEST_CASE(BishopAttacks) {
    struct {
        Square   square;
        Bitboard occ;
        Bitboard expected_attacks;

        void run() {
            EXPECT(bishop_attacks(square, occ)).to_be(expected_attacks);
        }
    } tests[] = {
        { SQ_D4, 0x200000000000ULL, 0x1221400142241ULL },
        { SQ_D4, 0x8000000000000000ULL, 0x8041221400142241ULL },
        { SQ_A1, 0, 0x8040201008040200ULL },
        { SQ_A1, 0x8000000000000000ULL, 0x8040201008040200ULL },
        { SQ_A1, 0x8040201008040201ULL, 0x200ULL },
        { SQ_A8, 0x102040810204080ULL, 0x2000000000000ULL },
    };

    for (auto& t: tests) {
        t.run();
    }
}

TEST_CASE(RookAttacks) {
    struct {
        Square   square;
        Bitboard occ;
        Bitboard expected_attacks;

        void run() {
            EXPECT(rook_attacks(square, occ)).to_be(expected_attacks);
        }
    } tests[] = {
        { SQ_D4, 0x200000000000ULL, 0x8080808f7080808ULL },
        { SQ_D4, 0x800000000ULL, 0x8f7080808ULL },
        { SQ_A1, 0, 0x1010101010101feULL },
    };

    for (auto& t: tests) {
        t.run();
    }
}