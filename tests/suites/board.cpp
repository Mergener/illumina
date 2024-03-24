#include "../litetest/litetest.h"

#include <iostream>

#include "board.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(Board);

TEST_CASE(SetPieceAt) {
    struct {
        Piece p;
        Square s;
        Board b {};

        void test() {
            // Collect values before board edition to compare later
            ui64 prev_key      = b.hash_key();
            Piece prev_piece   = b.piece_at(s);
            Bitboard prev_p_bb = b.piece_bb(p);
            Bitboard prev_prev_piece_bb = b.piece_bb(prev_piece);

            Bitboard prev_occ = b.occupancy();

            b.set_piece_at(s, p);

            // Get values after piece edition
            Bitboard prev_piece_bb = b.piece_bb(prev_piece);
            Bitboard p_bb = b.piece_bb(p);
            Bitboard occ  = b.occupancy();
            ui64 key      = b.hash_key();

            EXPECT(b.piece_at(s)).to_be(p);

            if (p != prev_piece) {
                EXPECT(key).to_not_be(prev_key);
                EXPECT(p_bb).to_be(set_bit(prev_p_bb, s));

                if (p == PIECE_NULL) {
                    EXPECT(occ).to_be(unset_bit(prev_occ, s));
                }
                else if (prev_piece == PIECE_NULL) {
                    EXPECT(occ).to_be(set_bit(prev_occ, s));
                }
                else {
                    EXPECT(occ).to_be(prev_occ);
                }
            }
            else {
                // Expect no changes.
                EXPECT(occ).to_be(prev_occ);
                EXPECT(p_bb).to_be(prev_p_bb);
                EXPECT(prev_piece_bb).to_be(prev_prev_piece_bb);
                EXPECT(key).to_be(prev_key);
            }
        }
    } s_cases[] = {
        { WHITE_PAWN, SQ_D4 }
    };

    for (auto& test_case: s_cases) {
        test_case.test();
    }
}

TEST_CASE(BoardFENConstructor) {
    // Test default constructor
    Board board_default("");
    EXPECT(board_default.color_to_move()).to_be(CL_WHITE);
    EXPECT(board_default.occupancy()).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_PAWN))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_KNIGHT))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_BISHOP))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_ROOK))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_QUEEN))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_WHITE, PT_KING))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_PAWN))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_KNIGHT))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_BISHOP))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_ROOK))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_QUEEN))).to_be(0ULL);
    EXPECT(board_default.piece_bb(Piece(CL_BLACK, PT_KING))).to_be(0ULL);
    EXPECT(board_default.ep_square()).to_be(SQ_NULL);
    EXPECT(board_default.rule50()).to_be(0);
    EXPECT(board_default.frc()).to_be(false);
    EXPECT(board_default.in_check()).to_be(false);
    EXPECT(board_default.in_double_check()).to_be(false);
    EXPECT(board_default.hash_key()).to_be(EMPTY_BOARD_HASH_KEY);
    EXPECT(board_default.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_default.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_default.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_default.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
//    EXPECT(board_default.legal()).to_be(true);

//    // Test constructor with FEN string representing the starting position
    Board board_startpos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT(board_startpos.color_to_move()).to_be(CL_WHITE);
    EXPECT(board_startpos.occupancy()).to_be(0xffff00000000ffffULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_PAWN))).to_be(0xff00ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_KNIGHT))).to_be(0x42ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_BISHOP))).to_be(0x24ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_ROOK))).to_be(0x81ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_QUEEN))).to_be(0x8ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_WHITE, PT_KING))).to_be(0x10ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_PAWN))).to_be(0xff000000000000ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_KNIGHT))).to_be(0x4200000000000000ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_BISHOP))).to_be(0x2400000000000000ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_ROOK))).to_be(0x8100000000000000ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_QUEEN))).to_be(0x800000000000000ULL);
    EXPECT(board_startpos.piece_bb(Piece(CL_BLACK, PT_KING))).to_be(0x1000000000000000ULL);
    EXPECT(board_startpos.ep_square()).to_be(SQ_NULL);
    EXPECT(board_startpos.frc()).to_be(false);
    EXPECT(board_startpos.in_check()).to_be(false);
    EXPECT(board_startpos.in_double_check()).to_be(false);
    EXPECT(board_startpos.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_startpos.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_startpos.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_startpos.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
//    EXPECT(board_startpos.legal()).to_be(true);

    // Test constructor with FEN string representing custom positions.
    Board board_custompos_1("7k/8/8/3K4/8/8/P7/8 b - - 32 1");
    EXPECT(board_custompos_1.color_to_move()).to_be(CL_BLACK);
    EXPECT(board_custompos_1.occupancy()).to_be(0x8000000800000100ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_PAWN))).to_be(0x100ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_KNIGHT))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_BISHOP))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_ROOK))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_QUEEN))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_WHITE, PT_KING))).to_be(0x800000000ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_PAWN))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_KNIGHT))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_BISHOP))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_ROOK))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_QUEEN))).to_be(0x0ULL);
    EXPECT(board_custompos_1.piece_bb(Piece(CL_BLACK, PT_KING))).to_be(0x8000000000000000ULL);
    EXPECT(board_custompos_1.ep_square()).to_be(SQ_NULL);
    EXPECT(board_custompos_1.rule50()).to_be(32);
    EXPECT(board_custompos_1.frc()).to_be(false);
    EXPECT(board_custompos_1.in_check()).to_be(false);
    EXPECT(board_custompos_1.in_double_check()).to_be(false);
    EXPECT(board_custompos_1.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_custompos_1.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_custompos_1.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_custompos_1.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
//    EXPECT(board_custompos_1.legal()).to_be(true);

    // Test FRC construction with FEN.
    Board frc_board_1("bnnrkrqb/pppppppp/8/8/8/8/PPPPPPPP/BNNRKRQB w KQkq - 0 1");
    EXPECT(frc_board_1.frc()).to_be(true);
    EXPECT(frc_board_1.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_F1);
    EXPECT(frc_board_1.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_D1);
    EXPECT(frc_board_1.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_F8);
    EXPECT(frc_board_1.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_D8);
    EXPECT(frc_board_1.castling_rights()).to_be(CR_ALL);
    EXPECT(frc_board_1.has_castling_rights(CL_WHITE, SIDE_KING)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_WHITE, SIDE_QUEEN)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_BLACK, SIDE_KING)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_BLACK, SIDE_QUEEN)).to_be(true);

    Board frc_board_2("bnnrkrqb/pppppppp/8/8/8/8/PPPPPPPP/BNNRKRQB w FDfd - 0 1");
    EXPECT(frc_board_1.frc()).to_be(true);
    EXPECT(frc_board_1.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_F1);
    EXPECT(frc_board_1.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_D1);
    EXPECT(frc_board_1.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_F8);
    EXPECT(frc_board_1.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_D8);
    EXPECT(frc_board_1.castling_rights()).to_be(CR_ALL);
    EXPECT(frc_board_1.has_castling_rights(CL_WHITE, SIDE_KING)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_WHITE, SIDE_QUEEN)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_BLACK, SIDE_KING)).to_be(true);
    EXPECT(frc_board_1.has_castling_rights(CL_BLACK, SIDE_QUEEN)).to_be(true);

    Board frc_board_3("bnnrkrqb/pppppppp/8/8/8/8/PPPPPPPP/BNNRKRQB w Kkq - 0 1");
    EXPECT(frc_board_3.frc()).to_be(true);
    EXPECT(frc_board_3.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_F1);
    EXPECT(frc_board_3.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_F8);
    EXPECT(frc_board_3.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_D8);
    EXPECT(frc_board_3.castling_rights()).to_be(CR_WHITE_OO | CR_BLACK_OO | CR_BLACK_OOO);
    EXPECT(frc_board_3.has_castling_rights(CL_WHITE, SIDE_KING)).to_be(true);
    EXPECT(frc_board_3.has_castling_rights(CL_WHITE, SIDE_QUEEN)).to_be(false);
    EXPECT(frc_board_3.has_castling_rights(CL_BLACK, SIDE_KING)).to_be(true);
    EXPECT(frc_board_3.has_castling_rights(CL_BLACK, SIDE_QUEEN)).to_be(true);

}