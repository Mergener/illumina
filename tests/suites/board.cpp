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
    EXPECT(board_default.in_check()).to_be(false);
    EXPECT(board_default.in_double_check()).to_be(false);
    EXPECT(board_default.hash_key()).to_be(EMPTY_BOARD_HASH_KEY);
    EXPECT(board_default.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_default.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_default.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_default.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
    EXPECT(board_default.legal()).to_be(false);

    // Test constructor with FEN string representing the starting position
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
    EXPECT(board_startpos.in_check()).to_be(false);
    EXPECT(board_startpos.in_double_check()).to_be(false);
    EXPECT(board_startpos.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_startpos.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_startpos.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_startpos.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
    EXPECT(board_startpos.legal()).to_be(true);

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
    EXPECT(board_custompos_1.in_check()).to_be(false);
    EXPECT(board_custompos_1.in_double_check()).to_be(false);
    EXPECT(board_custompos_1.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_H1);
    EXPECT(board_custompos_1.castle_rook_square(CL_WHITE, SIDE_QUEEN)).to_be(SQ_A1);
    EXPECT(board_custompos_1.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_H8);
    EXPECT(board_custompos_1.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_A8);
    EXPECT(board_custompos_1.legal()).to_be(true);

    // Test FRC construction with FEN.
    Board frc_board_1("bnnrkrqb/pppppppp/8/8/8/8/PPPPPPPP/BNNRKRQB w KQkq - 0 1");
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
    EXPECT(frc_board_3.castle_rook_square(CL_WHITE, SIDE_KING)).to_be(SQ_F1);
    EXPECT(frc_board_3.castle_rook_square(CL_BLACK, SIDE_KING)).to_be(SQ_F8);
    EXPECT(frc_board_3.castle_rook_square(CL_BLACK, SIDE_QUEEN)).to_be(SQ_D8);
    EXPECT(frc_board_3.castling_rights()).to_be(CR_WHITE_OO | CR_BLACK_OO | CR_BLACK_OOO);
    EXPECT(frc_board_3.has_castling_rights(CL_WHITE, SIDE_KING)).to_be(true);
    EXPECT(frc_board_3.has_castling_rights(CL_WHITE, SIDE_QUEEN)).to_be(false);
    EXPECT(frc_board_3.has_castling_rights(CL_BLACK, SIDE_KING)).to_be(true);
    EXPECT(frc_board_3.has_castling_rights(CL_BLACK, SIDE_QUEEN)).to_be(true);

}

TEST_CASE(MakeUndoNullMove) {
    struct {
        const char* fen;

        void run() {
            Board board(fen);
            if (board.in_check() || !board.legal()) {
                // Don't test null moves in check/illegal positions.
                return;
            }

            std::string start_fen = board.fen();
            ui64 start_key        = board.hash_key();
            Color start_color     = board.color_to_move();

            board.make_null_move();

            EXPECT(board.legal()).to_be(true);
            std::string null_fen = board.fen();
            ui64 null_key        = board.hash_key();
            Color null_color     = board.color_to_move();

            board.undo_null_move();

            EXPECT(board.legal()).to_be(true);
            std::string curr_fen = board.fen();
            ui64 curr_key        = board.hash_key();
            Color curr_color     = board.color_to_move();

            // Test against current values.
            EXPECT(curr_fen).to_be(start_fen);
            EXPECT(curr_color).to_be(start_color);
            EXPECT(curr_key).to_be(start_key);

            // Test against values during the null move.
            EXPECT(null_fen).to_not_be(start_fen);
            EXPECT(null_color).to_be(opposite_color(start_color));
            EXPECT(null_key).to_not_be(start_key);
        }

    } tests[] = {
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" },
        { "4k3/8/8/8/8/8/8/4K2R w K - 0 1" },
        { "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1" },
        { "4k2r/8/8/8/8/8/8/4K3 w k - 0 1" },
        { "r3k3/8/8/8/8/8/8/4K3 w q - 0 1" },
        { "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1" },
        { "r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1" },
        { "8/8/8/8/8/8/6k1/4K2R w K - 0 1" },
        { "8/8/8/8/8/8/1k6/R3K3 w Q - 0 1" },
        { "4k2r/6K1/8/8/8/8/8/8 w k - 0 1" },
        { "r3k3/1K6/8/8/8/8/8/8 w q - 0 1" },
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1" },
        { "1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1" },
        { "2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1" },
        { "r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1" },
        { "4k3/8/8/8/8/8/8/4K2R b K - 0 1" },
        { "4k3/8/8/8/8/8/8/R3K3 b Q - 0 1" },
        { "4k2r/8/8/8/8/8/8/4K3 b k - 0 1" },
        { "r3k3/8/8/8/8/8/8/4K3 b q - 0 1" },
        { "4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1" },
        { "r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1" },
        { "8/8/8/8/8/8/6k1/4K2R b K - 0 1" },
        { "8/8/8/8/8/8/1k6/R3K3 b Q - 0 1" },
        { "4k2r/6K1/8/8/8/8/8/8 b k - 0 1" },
        { "r3k3/1K6/8/8/8/8/8/8 b q - 0 1" },
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1" },
        { "r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1" },
        { "1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1" },
        { "2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1" },
        { "r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1" },
        { "8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1" },
        { "8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1" },
        { "8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1" },
        { "K7/8/2n5/1n6/8/8/8/k6N w - - 0 1" },
        { "k7/8/2N5/1N6/8/8/8/K6n w - - 0 1" },
        { "8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1" },
        { "8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1" },
        { "8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1" },
        { "K7/8/2n5/1n6/8/8/8/k6N b - - 0 1" },
        { "k7/8/2N5/1N6/8/8/8/K6n b - - 0 1" },
        { "B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1" },
        { "8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1" },
        { "k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1" },
        { "K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1" },
        { "B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1" },
        { "8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1" },
        { "k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1" },
        { "K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1" },
        { "7k/RR6/8/8/8/8/rr6/7K w - - 0 1" },
        { "R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1" },
        { "7k/RR6/8/8/8/8/rr6/7K b - - 0 1" },
        { "R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1" },
        { "6kq/8/8/8/8/8/8/7K w - - 0 1" },
        { "6KQ/8/8/8/8/8/8/7k b - - 0 1" },
        { "K7/8/8/3Q4/4q3/8/8/7k w - - 0 1" },
        { "6qk/8/8/8/8/8/8/7K b - - 0 1" },
        { "6KQ/8/8/8/8/8/8/7k b - - 0 1" },
        { "K7/8/8/3Q4/4q3/8/8/7k b - - 0 1" },
        { "8/8/8/8/8/K7/P7/k7 w - - 0 1" },
        { "8/8/8/8/8/7K/7P/7k w - - 0 1" },
        { "K7/p7/k7/8/8/8/8/8 w - - 0 1" },
        { "7K/7p/7k/8/8/8/8/8 w - - 0 1" },
        { "8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1" },
        { "8/8/8/8/8/K7/P7/k7 b - - 0 1" },
        { "8/8/8/8/8/7K/7P/7k b - - 0 1" },
        { "K7/p7/k7/8/8/8/8/8 b - - 0 1" },
        { "7K/7p/7k/8/8/8/8/8 b - - 0 1" },
        { "8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1" },
        { "8/8/8/8/8/4k3/4P3/4K3 w - - 0 1" },
        { "4k3/4p3/4K3/8/8/8/8/8 b - - 0 1" },
        { "8/8/7k/7p/7P/7K/8/8 w - - 0 1" },
        { "8/8/k7/p7/P7/K7/8/8 w - - 0 1" },
        { "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1" },
        { "8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1" },
        { "8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1" },
        { "k7/8/3p4/8/3P4/8/8/7K w - - 0 1" },
        { "8/8/7k/7p/7P/7K/8/8 b - - 0 1" },
        { "8/8/k7/p7/P7/K7/8/8 b - - 0 1" },
        { "8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1" },
        { "8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1" },
        { "8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1" },
        { "k7/8/3p4/8/3P4/8/8/7K b - - 0 1" },
        { "7k/3p4/8/8/3P4/8/8/K7 w - - 0 1" },
        { "7k/8/8/3p4/8/8/3P4/K7 w - - 0 1" },
        { "k7/8/8/7p/6P1/8/8/K7 w - - 0 1" },
        { "k7/8/7p/8/8/6P1/8/K7 w - - 0 1" },
        { "k7/8/8/6p1/7P/8/8/K7 w - - 0 1" },
        { "k7/8/6p1/8/8/7P/8/K7 w - - 0 1" },
        { "k7/8/8/3p4/4p3/8/8/7K w - - 0 1" },
        { "k7/8/3p4/8/8/4P3/8/7K w - - 0 1" },
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1" },
        { "7k/8/8/3p4/8/8/3P4/K7 b - - 0 1" },
        { "k7/8/8/7p/6P1/8/8/K7 b - - 0 1" },
        { "k7/8/7p/8/8/6P1/8/K7 b - - 0 1" },
        { "k7/8/8/6p1/7P/8/8/K7 b - - 0 1" },
        { "k7/8/6p1/8/8/7P/8/K7 b - - 0 1" },
        { "k7/8/8/3p4/4p3/8/8/7K b - - 0 1" },
        { "k7/8/3p4/8/8/4P3/8/7K b - - 0 1" },
        { "7k/8/8/p7/1P6/8/8/7K w - - 0 1" },
        { "7k/8/p7/8/8/1P6/8/7K w - - 0 1" },
        { "7k/8/8/1p6/P7/8/8/7K w - - 0 1" },
        { "7k/8/1p6/8/8/P7/8/7K w - - 0 1" },
        { "k7/7p/8/8/8/8/6P1/K7 w - - 0 1" },
        { "k7/6p1/8/8/8/8/7P/K7 w - - 0 1" },
        { "3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1" },
        { "7k/8/8/p7/1P6/8/8/7K b - - 0 1" },
        { "7k/8/p7/8/8/1P6/8/7K b - - 0 1" },
        { "7k/8/8/1p6/P7/8/8/7K b - - 0 1" },
        { "7k/8/1p6/8/8/P7/8/7K b - - 0 1" },
        { "k7/7p/8/8/8/8/6P1/K7 b - - 0 1" },
        { "k7/6p1/8/8/8/8/7P/K7 b - - 0 1" },
        { "3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1" },
        { "8/Pk6/8/8/8/8/6Kp/8 w - - 0 1" },
        { "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1" },
        { "8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1" },
        { "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1" },
        { "8/Pk6/8/8/8/8/6Kp/8 b - - 0 1" },
        { "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1" },
        { "8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1" },
        { "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1" },
    };

    for (auto& t: tests) {
        t.run();
    }
}

TEST_CASE(IsMovePseudoLegal) {
    {
        Board b("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1");
        EXPECT(b.is_move_pseudo_legal(Move::parse_uci(b, "e8g8"))).to_be(true);
    }
}

TEST_CASE(MoveGivesCheck) {
    struct {
        std::string fen;
        std::string move_str;
        bool gives_check;

        void run() {
            Board board(fen);
            Move move = Move::parse_uci(board, move_str);

            EXPECT(board.gives_check(move)).to_be(gives_check);
        }
    } tests[] = {
        { "rnbqkbnr/ppp2ppp/4p3/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R b KQkq - 1 3", "f8b4", true },
        { "rnbqkbnr/ppp2ppp/4p3/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R b KQkq - 1 3", "g8f6", false },
        { "8/2p2k2/8/8/8/5N2/1K3R2/8 w - - 0 1", "f3d4", true },
        { "8/2p2k2/8/8/8/5N2/1K3R2/8 w - - 0 1", "f3e5", true },
        { "8/2p2k2/5p2/8/8/5N2/1K3R2/8 w - - 0 1", "f3d4", false },
        { "8/2p2k2/5p2/8/8/5N2/1K3R2/8 w - - 0 1", "f3e5", true },
        { "8/5P2/5k2/8/8/2K5/8/8 w - - 0 1", "f7f8r", true },
        { "8/5P2/5k2/8/8/2K5/8/8 w - - 0 1", "f7f8q", true },
        { "8/5P2/5k2/8/8/2K5/8/8 w - - 0 1", "f7f8n", false },
        { "8/5P2/5k2/8/8/2K5/8/8 w - - 0 1", "f7f8b", false },
        { "8/8/8/1RPp1k2/8/8/2K5/8 w - d6 0 2", "c5d6", true },
    };

    for (auto& test: tests) {
        test.run();
    }
}