#ifndef ILUMINA_TYPES_H
#define ILUMINA_TYPES_H

#include <cstdint>

#include "debug.h"

//
// Some useful bit manipulation macros
//

#define BIT(n) ((1ULL) << (n))
#define BITMASK(nbits) ((1ul << (nbits)) - 1ul)

//
// The following headers contain required intrinsics to manipulate bits on a bitboard.
// We conditionally include them based on the compiler we're using.
//

#ifdef __GNUC__
#include <cpuid.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#endif

namespace ilumina {

/**
 * Initializes the types module.
 */
void init_types();

//
// Primitive integer types
//

using i8   = std::int8_t;
using i16  = std::int16_t;
using i32  = std::int32_t;
using i64  = std::int64_t;
using ui8  = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

//
// Bitboards
//

using Bitboard = ui64;

/**
 * Returns the position of the least significant bit from a given non-zero 64 bit integer.
 *
 * Example:
 * The number 4056 can be written as the following 64 bit big-endian sequence:
 * 00000000 00000000 00000000 00000000 00000000 00000000 00001111 11011000
 *
 * In this case, the least significant bit would be the 1 at the 4th position,
 * so this function would return 3 (zero-indexed).
 */
inline ui8 pop_lsb(ui64 n) {
    ILUMINA_ASSERT(n != 0);
#if defined(_MSC_VER)
    unsigned long idx;
	_BitScanForward64(&idx, n);
	return idx;
#elif defined(__GNUC__)
    return __builtin_ctzll(n);
#else
#error No bitscan function found.
#endif
}

/**
 * Returns the position of the least significant bit from a given non-zero 64 bit integer.
 *
 * Example:
 * The number 4056 can be written as the following 64 bit big-endian sequence:
 * 00000000 00000000 00000000 00000000 00000000 00000000 00001111 11011000
 *
 * In this case, the most significant bit would be the 1 at the 12th position,
 * so this function would return 11 (zero-indexed).
 */
inline ui8 pop_msb(ui64 n) {
    ILUMINA_ASSERT(n != 0);
#if defined(_MSC_VER)
    unsigned long idx;
	_BitScanReverse64(&idx, n);
	return idx;
#elif defined(__GNUC__)
    return 63 ^ __builtin_clzll(n);
#else
#error No bitscan function found.
#endif
}

/**
 * Given a 64 bit bitboard, a name for a ui8 iterator and a code block,
 * iterates through every set bit on the bitboard, storing them on the specified bit
 * variable.
 */
#define BB_FOREACH(bb, bit, code) {     \
	Bitboard _bb_copy = bb;             \
	while (_bb_copy) {                  \
		ui8 bit = pop_lsb(_bb_copy);    \
		code                            \
		_bb_copy &= _bb_copy - 1;       \
	}                                   \
}

//
// Colors
//

/**
 * Represents a color (black or white) in a chess game.
 * Named by CL_* constants.
 */
using Color = ui8;

enum {
    CL_WHITE,
    CL_BLACK,
    CL_COUNT
};

inline constexpr Color COLORS[] = { CL_WHITE, CL_BLACK };

inline constexpr Color opposite_color(Color c) { return c ^ 1; }

//
// Directions
//

/**
 * A direction on the 64-square board.
 * Named by DIR_* constants.
 * Can perform arithmetic with squares.
 */
using Direction = i8;

enum {
    DIR_NORTH     = 8,
    DIR_SOUTH     = -8,
    DIR_EAST      = 1,
    DIR_WEST      = -1,
    DIR_NORTHEAST = DIR_NORTH + DIR_EAST,
    DIR_NORTHWEST = DIR_NORTH + DIR_WEST,
    DIR_SOUTHEAST = DIR_SOUTH + DIR_EAST,
    DIR_SOUTHWEST = DIR_SOUTH + DIR_WEST
};

inline constexpr Direction DIRECTIONS[] = {
    DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST,
    DIR_NORTHEAST, DIR_NORTHWEST,
    DIR_SOUTHEAST, DIR_SOUTHWEST
};

inline constexpr Direction pawn_push_direction(Color color) {
    constexpr Direction PUSH_DIRS[] = { DIR_NORTH, DIR_SOUTH };
    return PUSH_DIRS[color];
}

//
// Squares
//

/**
 * Represents the index of a square on the board.
 * Named by SQ_* constants.
 */
using Square = ui8;

enum {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,

    SQ_NULL  = SQ_H8 + 1,
    SQ_COUNT = 64
};


inline constexpr Square mirror_horizontal(Square s) {
    constexpr Square MIRRORS[] {
        SQ_H1, SQ_G1, SQ_F1, SQ_E1, SQ_D1, SQ_C1, SQ_B1, SQ_A1,
        SQ_H2, SQ_G2, SQ_F2, SQ_E2, SQ_D2, SQ_C2, SQ_B2, SQ_A2,
        SQ_H3, SQ_G3, SQ_F3, SQ_E3, SQ_D3, SQ_C3, SQ_B3, SQ_A3,
        SQ_H4, SQ_G4, SQ_F4, SQ_E4, SQ_D4, SQ_C4, SQ_B4, SQ_A4,
        SQ_H5, SQ_G5, SQ_F5, SQ_E5, SQ_D5, SQ_C5, SQ_B5, SQ_A5,
        SQ_H6, SQ_G6, SQ_F6, SQ_E6, SQ_D6, SQ_C6, SQ_B6, SQ_A6,
        SQ_H7, SQ_G7, SQ_F7, SQ_E7, SQ_D7, SQ_C7, SQ_B7, SQ_A7,
        SQ_H8, SQ_G8, SQ_F8, SQ_E8, SQ_D8, SQ_C8, SQ_B8, SQ_A8,
    };

    return MIRRORS[s];
}

inline constexpr Square mirror_vertical(Square s) {
    constexpr Square MIRRORS[] {
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    };

    return MIRRORS[s];
}

inline int chebyshev_distance(Square a, Square b) {
    extern int g_ChebyshevDistances[SQ_COUNT][SQ_COUNT];
    return g_ChebyshevDistances[a][b];
}

inline int manhattan_distance(Square a, Square b) {
    extern int g_ManhattanDistances[SQ_COUNT][SQ_COUNT];
    return g_ManhattanDistances[a][b];
}

inline static constexpr Square pawn_push_destination(Square src, Color color) {
    return src + pawn_push_direction(color);
}

inline static constexpr Square double_push_destination(Square src, Color color) {
    return src + pawn_push_direction(color) * 2;
}

//
// Pieces
//

using PieceType = ui8;

enum {
    PT_NULL,
    PT_PAWN,
    PT_KNIGHT,
    PT_BISHOP,
    PT_ROOK,
    PT_QUEEN,
    PT_KING,
    PT_COUNT
};

inline constexpr PieceType PIECE_TYPES[] = {
    PT_PAWN, PT_KNIGHT,
    PT_BISHOP, PT_ROOK,
    PT_QUEEN, PT_KING
};

class Piece {
public:
    inline constexpr Piece(Color color, PieceType type)
        : m_data((color & BITMASK(1)) | ((type & BITMASK(3)) << 1)) {}

    inline explicit constexpr Piece(ui8 data)
        : m_data(data) {}

    inline constexpr Color color() const    { return m_data & BITMASK(1); }
    inline constexpr PieceType type() const { return (m_data >> 1); }

    inline constexpr ui8 raw() const { return m_data; }

private:
    ui8 m_data;
};

inline constexpr Piece WHITE_PAWN(CL_WHITE, PT_PAWN);
inline constexpr Piece WHITE_KNIGHT(CL_WHITE, PT_KNIGHT);
inline constexpr Piece WHITE_BISHOP(CL_WHITE, PT_BISHOP);
inline constexpr Piece WHITE_ROOK(CL_WHITE, PT_ROOK);
inline constexpr Piece WHITE_QUEEN(CL_WHITE, PT_QUEEN);
inline constexpr Piece WHITE_KING(CL_WHITE, PT_KING);

inline constexpr Piece BLACK_PAWN(CL_BLACK, PT_PAWN);
inline constexpr Piece BLACK_KNIGHT(CL_BLACK, PT_KNIGHT);
inline constexpr Piece BLACK_BISHOP(CL_BLACK, PT_BISHOP);
inline constexpr Piece BLACK_ROOK(CL_BLACK, PT_ROOK);
inline constexpr Piece BLACK_QUEEN(CL_BLACK, PT_QUEEN);
inline constexpr Piece BLACK_KING(CL_BLACK, PT_KING);

//
// Moves
//

using MoveType = ui8;
enum {
    MT_NORMAL,
    MT_SIMPLE_CAPTURE,
    MT_PROMOTION_CAPTURE,
    MT_EN_PASSANT,
    MT_DOUBLE_PUSH,
    MT_CASTLES,
    MT_SIMPLE_PROMOTION,
};

class Move {

    //
    // Encoding:
    //
    //   0 - 5:     Source square
    //   6 - 11:    Destination square
    //  12 - 15:    Source piece
    //  16 - 19:    Captured piece
    //  20 - 22:    Move type
    //  23 - 25:    Promotion piece type
    //  26 - 31:    Castles rook square
    //

public:
    inline explicit constexpr Move(ui32 data)
        : m_data(data) {}

    inline constexpr Square source() const                  { return m_data & BITMASK(6); }
    inline constexpr Square destination() const             { return (m_data >> 6) & BITMASK(6); }
    inline constexpr Piece source_piece() const             { return Piece((m_data >> 12) & BITMASK(4)); }
    inline constexpr Piece captured_piece() const           { return Piece((m_data >> 16) & BITMASK(4)); }
    inline constexpr MoveType type() const                  { return (m_data >> 20) & BITMASK(3); }
    inline constexpr PieceType promotion_piece_type() const { return (m_data >> 23) & BITMASK(3); }
    inline constexpr Square castles_rook_square() const     { return (m_data >> 26) & BITMASK(6); }

    inline constexpr bool is_capture() const {
        constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE) | BIT(MT_EN_PASSANT) | BIT(MT_PROMOTION_CAPTURE);
        return (BIT(type()) & MASK) != 0;
    }

private:
    ui32 m_data;

    inline static constexpr Move base(Square source, Square dest, Piece src_piece, MoveType type) {
        Move move(0);

        move.m_data |= (source & BITMASK(6))          << 0;
        move.m_data |= (dest & BITMASK(6))            << 6;
        move.m_data |= (src_piece.raw() & BITMASK(4)) << 12;
        move.m_data |= (type & BITMASK(3))            << 20;

        return move;
    }

public:
    inline static constexpr Move new_normal(Square src,
                                            Square dst,
                                            Piece src_piece) {
        return Move::base(src, dst, src_piece, MT_NORMAL);
    }

    inline static constexpr Move new_simple_capture(Square src,
                                                    Square dst,
                                                    Piece src_piece,
                                                    Piece capt_piece) {
        Move move = Move::base(src, dst, src_piece, MT_SIMPLE_CAPTURE);
        move.m_data |= (capt_piece.raw() & BITMASK(4)) << 16;
        return move;
    }

    inline static constexpr Move new_promotion_capture(Square src,
                                                       Square dst,
                                                       Color pawn_color,
                                                       Piece capt_piece,
                                                       Piece prom_piece) {
        Move move = Move::base(src, dst, Piece(pawn_color, PT_PAWN), MT_PROMOTION_CAPTURE);
        move.m_data |= (capt_piece.raw() & BITMASK(4)) << 16;
        move.m_data |= (prom_piece.raw() & BITMASK(4)) << 23;
        return move;
    }

    inline static constexpr Move new_en_passant_capture(Square src,
                                                        Square dst,
                                                        Color pawn_color) {
        Move move = Move::base(src, dst, Piece(pawn_color, PT_PAWN), MT_EN_PASSANT);
        move.m_data |= (Piece(opposite_color(pawn_color), PT_PAWN).raw() & BITMASK(4)) << 16;
        return move;
    }

    inline static constexpr Move new_double_push(Square src, Color pawn_color) {
        Move move = Move::base(src, pawn_push_destination(src, pawn_color), Piece(pawn_color, PT_PAWN), MT_DOUBLE_PUSH);
        return move;
    }

    inline static constexpr Move new_castles(Square src, Square dst, Color king_color, Square rook_square) {
        Move move = Move::base(src, dst, Piece(king_color, PT_KING), rook_square);
        move.m_data |= (rook_square & BITMASK(6)) << 26;
        return move;
    }

    inline static constexpr Move new_simple_promotion(Square src,
                                                      Square dst,
                                                      Color pawn_color,
                                                      Piece prom_piece) {
        Move move = Move::base(src, dst, Piece(pawn_color, PT_PAWN), MT_SIMPLE_PROMOTION);
        move.m_data |= (prom_piece.raw() & BITMASK(4)) << 23;
        return move;
    }
};

inline constexpr Move MOVE_NULL(0);

} // ilumina

#endif // ILUMINA_TYPES_H
