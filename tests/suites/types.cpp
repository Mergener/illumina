#include "../litetest/litetest.h"

#include <iostream>

#include "types.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(Types);

SUITE_SETUP(Types) {
    init_types();
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
        { WHITE_PAWN, PT_PAWN, CL_WHITE, 'P' },
        { WHITE_KNIGHT, PT_KNIGHT, CL_WHITE, 'N' },
        { WHITE_BISHOP, PT_BISHOP, CL_WHITE, 'B' },
        { WHITE_ROOK, PT_ROOK, CL_WHITE, 'R' },
        { WHITE_QUEEN, PT_QUEEN, CL_WHITE, 'Q' },
        { WHITE_KING, PT_KING, CL_WHITE, 'K' },

        { BLACK_PAWN, PT_PAWN, CL_BLACK, 'p' },
        { BLACK_KNIGHT, PT_KNIGHT, CL_BLACK, 'n' },
        { BLACK_BISHOP, PT_BISHOP, CL_BLACK, 'b' },
        { BLACK_ROOK, PT_ROOK, CL_BLACK, 'r' },
        { BLACK_QUEEN, PT_QUEEN, CL_BLACK, 'q' },
        { BLACK_KING, PT_KING, CL_BLACK, 'k' },
    };

    for (const auto& test_case: cases) {
        test_case.test();
    }
}

TEST_CASE(MoveConstruction) {
    // Test constructing a normal move
    Move normal_move = Move::new_normal(SQ_E2, SQ_E4, Piece(CL_WHITE, PT_PAWN));
    EXPECT(normal_move.source()).to_be(SQ_E2);
    EXPECT(normal_move.destination()).to_be(SQ_E4);
    EXPECT(normal_move.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(normal_move.type()).to_be(MT_NORMAL);

    // Test constructing a simple capture move
    Move capture = Move::new_simple_capture(SQ_D4, SQ_E5, Piece(CL_WHITE, PT_PAWN), Piece(CL_BLACK, PT_KNIGHT));
    EXPECT(capture.source()).to_be(SQ_D4);
    EXPECT(capture.destination()).to_be(SQ_E5);
    EXPECT(capture.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(capture.captured_piece()).to_be(Piece(CL_BLACK, PT_KNIGHT));
    EXPECT(capture.type()).to_be(MT_SIMPLE_CAPTURE);

    // Test constructing a promotion capture move
    Move promotion_capture = Move::new_promotion_capture(SQ_G7, SQ_H8, CL_BLACK, Piece(CL_WHITE, PT_PAWN), PT_QUEEN);
    EXPECT(promotion_capture.source()).to_be(SQ_G7);
    EXPECT(promotion_capture.destination()).to_be(SQ_H8);
    EXPECT(promotion_capture.source_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(promotion_capture.captured_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(promotion_capture.promotion_piece_type()).to_be(PT_QUEEN);
    EXPECT(promotion_capture.type()).to_be(MT_PROMOTION_CAPTURE);

    // Test constructing an en passant capture move
    Move en_passant = Move::new_en_passant_capture(SQ_D5, SQ_E6, CL_WHITE);
    EXPECT(en_passant.source()).to_be(SQ_D5);
    EXPECT(en_passant.destination()).to_be(SQ_E6);
    EXPECT(en_passant.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(en_passant.captured_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(en_passant.type()).to_be(MT_EN_PASSANT);

    // Test constructing a double push move
    Move double_push = Move::new_double_push(SQ_G2, CL_WHITE);
    EXPECT(double_push.source()).to_be(SQ_G2);
    EXPECT(double_push.destination()).to_be(SQ_G4);
    EXPECT(double_push.source_piece()).to_be(Piece(CL_WHITE, PT_PAWN));
    EXPECT(double_push.type()).to_be(MT_DOUBLE_PUSH);

    // Test constructing a castling move
    Move castles = Move::new_castles(SQ_E1, SQ_G1, CL_WHITE, SQ_H1);
    EXPECT(castles.source()).to_be(SQ_E1);
    EXPECT(castles.destination()).to_be(SQ_G1);
    EXPECT(castles.source_piece()).to_be(Piece(CL_WHITE, PT_KING));
    EXPECT(castles.castles_rook_square()).to_be(SQ_H1);
    EXPECT(castles.type()).to_be(MT_CASTLES);

    // Test constructing a simple promotion move
    Move promotion = Move::new_simple_promotion(SQ_H7, SQ_H8, CL_BLACK, PT_QUEEN);
    EXPECT(promotion.source()).to_be(SQ_H7);
    EXPECT(promotion.destination()).to_be(SQ_H8);
    EXPECT(promotion.source_piece()).to_be(Piece(CL_BLACK, PT_PAWN));
    EXPECT(promotion.promotion_piece_type()).to_be(PT_QUEEN);
    EXPECT(promotion.type()).to_be(MT_SIMPLE_PROMOTION);
}

