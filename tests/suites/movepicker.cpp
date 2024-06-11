#include "../litetest/litetest.h"

#include "movepicker.h"
#include "utils.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(MovePicker);

template <bool QUIESCE>
static void score_move(MovePicker<QUIESCE>& mp, SearchMove& move, bool check) {
    ui64 mask = !check
        ? BIT(MPS_GOOD_CAPTURES) | BIT(MPS_BAD_CAPTURES) | BIT(MPS_QUIET)
        : BIT(MPS_QUIET_EVASIONS) | BIT(MPS_NOISY_EVASIONS);

    if (!bit_is_set(mask, mp.stage())) {
        move.set_value(0);
    }
    else {
        mp.score_move(move);
    }
}

static MovePickingStage classify_move_stage(Move move,
                                            const Board& board,
                                            Move hash_move,
                                            MoveHistory mv_hist,
                                            Depth ply,
                                            bool quiesce) {
    if (move == hash_move) {
        return MPS_HASH_MOVE;
    }
    if (!board.in_check()) {
        MovePickingStage expected_stage;

        switch (move.type()) {
            case MT_PROMOTION_CAPTURE:
                expected_stage = MPS_PROMOTION_CAPTURES;
                break;

            case MT_SIMPLE_PROMOTION:
                expected_stage = MPS_PROMOTIONS;
                break;

            case MT_SIMPLE_CAPTURE:
                expected_stage = has_good_see(board, move.source(), move.destination()) ? MPS_GOOD_CAPTURES : MPS_BAD_CAPTURES;
                break;

            case MT_EN_PASSANT:
                expected_stage = MPS_EP;
                break;

            case MT_NORMAL:
            case MT_DOUBLE_PUSH:
            case MT_CASTLES:
                expected_stage = MPS_QUIET;
                break;
        }

        if (!quiesce && mv_hist.is_killer(ply, move)) {
            return std::min(MovePickingStage(MPS_KILLER_MOVES), expected_stage);
        }
        return expected_stage;
    }
    MovePickingStage expected_stage;
    switch (move.type()) {
        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
        case MT_SIMPLE_CAPTURE:
        case MT_EN_PASSANT:
            expected_stage = MPS_NOISY_EVASIONS;
            break;
        case MT_NORMAL:
        case MT_DOUBLE_PUSH:
        case MT_CASTLES:
            expected_stage = MPS_QUIET_EVASIONS;
            break;
    }

    if (!quiesce && mv_hist.is_killer(ply, move)) {
        return std::min(MovePickingStage(MPS_KILLER_EVASIONS), expected_stage);
    }
    return expected_stage;
}

template <bool QUIESCE, bool METAMORPHIC>
void test_move_picker() {
    constexpr ui64 MOVE_TYPE_MASK = QUIESCE
                                    ? ui64(BIT(MT_SIMPLE_CAPTURE) | BIT(MT_PROMOTION_CAPTURE) | BIT(MT_SIMPLE_PROMOTION) | BIT(MT_EN_PASSANT))
                                    : UINT64_MAX;
    struct {
        std::string fen;
        std::string hash_move_str {};
        Depth ply {};
        std::vector<std::pair<Depth, std::string>> all_killer_moves {};
        std::vector<std::tuple<Depth, Square, Square>> history {};

        void run() {
            Board board(fen);

            // Generate all moves without the move picker. Use the number of generated
            // moves to afterwards validate if the move picker generated the exact number
            // of moves too expected for the position.
            Move validation_moves[MAX_GENERATED_MOVES];
            Move* validation_moves_end = generate_moves<MOVE_TYPE_MASK>(board, validation_moves);
            size_t n_expected_moves = validation_moves_end - validation_moves;

            Move hash_move;
            MoveHistory mv_hist;

            if constexpr (!METAMORPHIC) {
                if (n_expected_moves > 0) {
                    hash_move = !QUIESCE ? Move::parse_uci(board, hash_move_str) : MOVE_NULL;

                    // Save killer moves.
                    for (auto& k: all_killer_moves) {
                        Move move = Move::parse_uci(board, k.second);
                        if (move != MOVE_NULL) {
                            mv_hist.set_killer(k.first, move);
                        }
                    }

                    // Save all history.
                    for (auto& h: history) {
                        auto [depth, src, dest] = h;
                        mv_hist.update_quiet_history(Move::new_normal(src, dest, WHITE_QUEEN), depth, true);
                    }
                }
            }
            else {
                if (n_expected_moves > 0) {
                    // Choose random hash move.
                    hash_move = random_bool()
                                ? validation_moves[random(size_t(0), n_expected_moves)]
                                : MOVE_NULL;

                    // For killer moves, use two branches: one with valid and legal killer moves,
                    // another with completely random ones.
                    if (random_bool()) {
                        // Valid and legal killer moves.
                        for (int i = 0; i < 2; ++i) {
                            Move move = validation_moves[random(size_t(0), n_expected_moves)];
                            if (move.is_quiet()) {
                                mv_hist.set_killer(ply, move);
                            }
                        }
                    }
                    else {
                        // Completely random moves.
                        Square src = random_square();
                        Square dst = random_square();
                        Move move = Move(board, src, dst);
                        if (move.is_quiet()) {
                            mv_hist.set_killer(ply, move);
                        }
                    }

                    // Fill history values with random depths [1,15) (arbitrary range) for each move.
                    // Arbitrarily do 100 random steps.
                    for (int i = 0; i < 100; ++i) {
                        // Pick a move.
                        Move move = validation_moves[random(size_t(0), n_expected_moves)];
                        mv_hist.update_quiet_history(move, random(1, 15), true);
                    }
                }
                else {
                    hash_move = MOVE_NULL;
                }
            }

            MovePicker<QUIESCE> move_picker(board, ply, mv_hist, hash_move);
            SearchMove move {};
            std::vector<SearchMove> mp_moves;
            MovePickingStage highest_stage = MPS_NOT_STARTED;
            int prev_score = INT32_MAX;
            bool check = board.in_check();
            while ((move = move_picker.next()) != MOVE_NULL) {
                mp_moves.push_back(move);
                MovePickingStage move_stage = classify_move_stage(move, board, hash_move, mv_hist, ply, QUIESCE);
                EXPECT(move_stage).to_be_greater_than_or_equal_to(highest_stage);

                if (move_stage > highest_stage) {
                    highest_stage = move_stage;
                    prev_score = INT32_MAX;
                }

                score_move(move_picker, move, check);

                // Make sure that every new move on a stage has a lower value than
                // the previous one (in other words: are being selected in a sorted
                // fashion).
                EXPECT(move.value()).to_be_less_than_or_equal_to(prev_score);
                prev_score = move.value();
            }

            // We validated whether the moves are being generated in order or not. Now, we need to validate whether
            // all moves have been generated correctly and no illegal moves have been generated.
            EXPECT(int(mp_moves.size())).to_be(int(n_expected_moves));
            for (SearchMove mp_move: mp_moves) {
                EXPECT(mp_move).to_not_be(MOVE_NULL);

                bool exists_in_legal_moves = false;
                for (Move* legal_it = &validation_moves[0]; legal_it != validation_moves_end; ++legal_it) {
                    Move lg_move = *legal_it;
                    if (lg_move == mp_move) {
                        exists_in_legal_moves = true;
                        *legal_it = MOVE_NULL; // Prevent same move from being counted twice.
                        break;
                    }
                }

                EXPECT(exists_in_legal_moves).to_be(true);
            }
        }
    } tests[] = {
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "", 0, {  }, {} },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "a2a3", 0, {  }, {} },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "a2a3", 0, { { 0, "b2b3" } }, {} },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "a2a3", 0, { { 0, "b2b3" } }, { { 10, SQ_G2, SQ_G4 } } },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "a2a3", 0, { { 0, "b2b3" } }, { { 10, SQ_G2, SQ_G4 }, { 12, SQ_D5, SQ_D6 } } },
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", "h8h7", 0, { { 0, "b2b3" } }, {} },
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", "h8h7", 0, { { 0, "d7d5" } }, {} },
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", "h8h7", 0, { { 0, "" } }, { { 3, SQ_H8, SQ_G8 } } },
        { "4k2r/8/8/8/8/8/8/4K3 w k - 0 1", {}, 0, {}, {}},
        { "r3k3/8/8/8/8/8/8/4K3 w q - 0 1", {}, 0, {}, {}},
        { "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/8/6k1/4K2R w K - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", {}, 0, {}, {}},
        { "4k2r/6K1/8/8/8/8/8/8 w k - 0 1", {}, 0, {}, {}},
        { "r3k3/1K6/8/8/8/8/8/8 w q - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", {}, 0, {}, {}},
        { "1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", {}, 0, {}, {}},
        { "2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", {}, 0, {}, {}},
        { "r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", {}, 0, {}, {}},
        { "4k3/8/8/8/8/8/8/4K2R b K - 0 1", {}, 0, {}, {}},
        { "4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", {}, 0, {}, {}},
        { "4k2r/8/8/8/8/8/8/4K3 b k - 0 1", {}, 0, {}, {}},
        { "r3k3/8/8/8/8/8/8/4K3 b q - 0 1", {}, 0, {}, {}},
        { "4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/8/6k1/4K2R b K - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", {}, 0, {}, {}},
        { "4k2r/6K1/8/8/8/8/8/8 b k - 0 1", {}, 0, {}, {}},
        { "r3k3/1K6/8/8/8/8/8/8 b q - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", {}, 0, {}, {}},
        { "1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", {}, 0, {}, {}},
        { "2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", {}, 0, {}, {}},
        { "r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", {}, 0, {}, {}},
        { "8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", {}, 0, {}, {}},
        { "8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", {}, 0, {}, {}},
        { "8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", {}, 0, {}, {}},
        { "K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", {}, 0, {}, {}},
        { "k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", {}, 0, {}, {}},
        { "8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", {}, 0, {}, {}},
        { "8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", {}, 0, {}, {}},
        { "8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", {}, 0, {}, {}},
        { "K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", {}, 0, {}, {}},
        { "k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", {}, 0, {}, {}},
        { "B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", {}, 0, {}, {}},
        { "8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", {}, 0, {}, {}},
        { "k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", {}, 0, {}, {}},
        { "K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", {}, 0, {}, {}},
        { "B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", {}, 0, {}, {}},
        { "8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", {}, 0, {}, {}},
        { "k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", {}, 0, {}, {}},
        { "K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", {}, 0, {}, {}},
        { "7k/RR6/8/8/8/8/rr6/7K w - - 0 1", {}, 0, {}, {}},
        { "R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", {}, 0, {}, {}},
        { "7k/RR6/8/8/8/8/rr6/7K b - - 0 1", {}, 0, {}, {}},
        { "R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", {}, 0, {}, {}},
        { "6kq/8/8/8/8/8/8/7K w - - 0 1", {}, 0, {}, {}},
        { "6KQ/8/8/8/8/8/8/7k b - - 0 1", {}, 0, {}, {}},
        { "K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", {}, 0, {}, {}},
        { "6qk/8/8/8/8/8/8/7K b - - 0 1", {}, 0, {}, {}},
        { "6KQ/8/8/8/8/8/8/7k b - - 0 1", {}, 0, {}, {}},
        { "K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/K7/P7/k7 w - - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/7K/7P/7k w - - 0 1", {}, 0, {}, {}},
        { "K7/p7/k7/8/8/8/8/8 w - - 0 1", {}, 0, {}, {}},
        { "7K/7p/7k/8/8/8/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/K7/P7/k7 b - - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/7K/7P/7k b - - 0 1", {}, 0, {}, {}},
        { "K7/p7/k7/8/8/8/8/8 b - - 0 1", {}, 0, {}, {}},
        { "7K/7p/7k/8/8/8/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", {}, 0, {}, {}},
        { "4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/8/7k/7p/7P/7K/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/8/k7/p7/P7/K7/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", {}, 0, {}, {}},
        { "8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/3p4/8/3P4/8/8/7K w - - 0 1", {}, 0, {}, {}},
        { "8/8/7k/7p/7P/7K/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/8/k7/p7/P7/K7/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", {}, 0, {}, {}},
        { "8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/3p4/8/3P4/8/8/7K b - - 0 1", {}, 0, {}, {}},
        { "7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/7p/6P1/8/8/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/7p/8/8/6P1/8/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/6p1/7P/8/8/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/6p1/8/8/7P/8/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/3p4/4p3/8/8/7K w - - 0 1", {}, 0, {}, {}},
        { "k7/8/3p4/8/8/4P3/8/7K w - - 0 1", {}, 0, {}, {}},
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/7p/6P1/8/8/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/7p/8/8/6P1/8/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/6p1/7P/8/8/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/6p1/8/8/7P/8/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/8/8/3p4/4p3/8/8/7K b - - 0 1", {}, 0, {}, {}},
        { "k7/8/3p4/8/8/4P3/8/7K b - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/p7/1P6/8/8/7K w - - 0 1", {}, 0, {}, {}},
        { "7k/8/p7/8/8/1P6/8/7K w - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/1p6/P7/8/8/7K w - - 0 1", {}, 0, {}, {}},
        { "7k/8/1p6/8/8/P7/8/7K w - - 0 1", {}, 0, {}, {}},
        { "k7/7p/8/8/8/8/6P1/K7 w - - 0 1", {}, 0, {}, {}},
        { "k7/6p1/8/8/8/8/7P/K7 w - - 0 1", {}, 0, {}, {}},
        { "3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/p7/1P6/8/8/7K b - - 0 1", {}, 0, {}, {}},
        { "7k/8/p7/8/8/1P6/8/7K b - - 0 1", {}, 0, {}, {}},
        { "7k/8/8/1p6/P7/8/8/7K b - - 0 1", {}, 0, {}, {}},
        { "7k/8/1p6/8/8/P7/8/7K b - - 0 1", {}, 0, {}, {}},
        { "k7/7p/8/8/8/8/6P1/K7 b - - 0 1", {}, 0, {}, {}},
        { "k7/6p1/8/8/8/8/7P/K7 b - - 0 1", {}, 0, {}, {}},
        { "3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", {}, 0, {}, {}},
        { "8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", {}, 0, {}, {}},
        { "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", {}, 0, {}, {}},
        { "8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", {}, 0, {}, {}},
        { "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", {}, 0, {}, {}},
        { "8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", {}, 0, {}, {}},
        { "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", {}, 0, {}, {}},
        { "8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", {}, 0, {}, {}},
        { "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", {}, 0, {}, {}},
        { "7r/6p1/3p2pp/pn4k1/3P3P/P2BP3/1r3PP1/3RK2R b K h3 0 19", {}, 0, {}, {}},
        { "r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", "", 0, { { 0, "e8g8" }, { 0, "a8b8" } }, {} },
        { "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", "h8g8", 0, { { 0, "d7d5" }, { 0, "h8g8" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, {}, {}},
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, { {0, "g8f8"} }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", {}, 0, { {0, "g8f8"} }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", {}, 0, { {0, "g8f8"}, { 0, "g8f8" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", {}, 0, { {}, { 0, "g8f8" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, { {0, "g8f8"} }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, { {0, "g8f8"}, { 0, "g8f8" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, { {}, { 0, "g8f8" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", {}, 0, { {0, "g8f8"} }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", {}, 0, { {0, "g8f8"}, { 0, "f6g7" } }, {} },
        { "2r1b1k1/1p3p1Q/3rpqp1/p3N1N1/Pb1Pp3/1P2P1P1/5PP1/1R1R2K1 b - - 3 30", "g8f8", 0, { {0, "g8f8"}, { 0, "f6g7" } }, {} },
    };

    for (auto& test: tests) {
        test.run();
    }
}

TEST_CASE(MovePickerTests) {
    test_move_picker<false, false>();
}

TEST_CASE(MovePickerQuiesceTests) {
    test_move_picker<true, false>();
}

TEST_CASE(MovePickerTestsMetamorphic) {
    test_move_picker<false, true>();
}

TEST_CASE(MovePickerQuiesceTestsMetamorphic) {
    test_move_picker<true, true>();
}