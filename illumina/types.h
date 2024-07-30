#ifndef ILLUMINA_TYPES_H
#define ILLUMINA_TYPES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <iostream>

#include "debug.h"

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

namespace illumina {

//
// Type definitions
//

// Primitive integer types

using i8   = std::int8_t;
using i16  = std::int16_t;
using i32  = std::int32_t;
using i64  = std::int64_t;
using ui8  = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

// Chess related types

using Bitboard       = ui64;
using Color          = ui8;
using Side           = ui8;
using CastlingRights = ui8;
using BoardRank      = int;
using BoardFile      = int;
using Direction      = int;
using Square         = int;
using PieceType      = ui8;
using MoveType       = ui8;

//
// Some useful bit manipulation functions/macros.
//

/**
 * Returns an integer with only the nth bit set.
 * Example: BIT(3) = 0b1000
 */
#define BIT(n) ((1ULL) << (n))

/**
 * Returns an integer with the first n bits set.
 * Example: BITMASK(5) = 0b11111
 */
#define BITMASK(n) ((1ULL << (n)) - 1ULL)

/**
 * Returns 'val', but with its nth bit unset.
 * Example: unset_bit(0b100101, 2) = 0b100001
 */
constexpr ui64 unset_bit(ui64 val, ui8 n) {
    return val & ~BIT(n);
}

/**
 * Returns 'val', but with its nth bit set.
 * Example: unset_bit(0b100101, 4) = 0b110101
 */
constexpr ui64 set_bit(ui64 val, ui8 n) {
    return val | BIT(n);
}

constexpr bool bit_is_set(ui64 val, ui8 bit) {
    return (val & BIT(bit)) != 0;
}

/**
 * Rotates bits leftwise.
 * Example: lrot(0b00110010, 4) = 0b00100011
 */
constexpr ui8 lrot(ui8 val, ui8 rot) {
    return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

/**
 * Taken from https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#FlipVertically
 */
constexpr ui64 flip_bits_vert(ui64 x) {
    const ui64 k1 = 0x00FF00FF00FF00FFULL;
    const ui64 k2 = 0x0000FFFF0000FFFFULL;
    x = ((x >>  8) & k1) | ((x & k1) <<  8);
    x = ((x >> 16) & k2) | ((x & k2) << 16);
    x = ( x >> 32)       | ( x       << 32);
    return x;
}

inline ui64 popcount(ui64 x) {
#ifdef __GNUC__
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    return __popcnt64(x);
#endif
}

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
inline ui8 lsb(ui64 n) {
    ILLUMINA_ASSERT(n != 0);
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
inline ui8 msb(ui64 n) {
    ILLUMINA_ASSERT(n != 0);
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

constexpr ui64 unset_lsb(ui64 x) {
    return x & (x - 1);
}

inline constexpr Bitboard DARK_SQUARES  = 0xaa55aa55aa55aa55ULL;
inline constexpr Bitboard LIGHT_SQUARES = ~DARK_SQUARES;

//
// Colors
//

enum {
    CL_WHITE,
    CL_BLACK,
    CL_COUNT
};

#define ILLUMINA_ASSERT_VALID_COLOR(c) ILLUMINA_ASSERT((c) == illumina::CL_WHITE || (c) == illumina::CL_BLACK)

constexpr Color COLORS[] = { CL_WHITE, CL_BLACK };

constexpr Color opposite_color(Color c) { return c ^ 1; }

inline Color color_from_char(char c) {
    ILLUMINA_ASSERT(c == 'w' || c == 'W' || c == 'b' || c == 'B');

    c = std::tolower(c);

    if (c == 'b') {
        return CL_BLACK;
    }
    return CL_WHITE;
}

constexpr char color_to_char(Color color) {
    ILLUMINA_ASSERT_VALID_COLOR(color);
    return color == CL_WHITE
                  ? 'w'
                  : 'b';
}

constexpr const char* color_name(Color color) {
    ILLUMINA_ASSERT_VALID_COLOR(color);
    return color == CL_WHITE
                  ? "white"
                  : "black";
}

//
// Board sides
//

enum {
    SIDE_KING,
    SIDE_QUEEN,
    SIDE_COUNT = 2
};

constexpr Side SIDES[] = { SIDE_KING, SIDE_QUEEN };

//
// Castle rights
//

constexpr CastlingRights make_castling_rights(bool white_king_side,
                                                     bool white_queen_side,
                                                     bool black_king_side,
                                                     bool black_queen_side) {
    return (white_king_side) | (white_queen_side << 1)
         | (black_king_side) | (black_queen_side);
}

enum {
    CR_NONE      = 0,
    CR_WHITE_OO  = BIT(0),
    CR_WHITE_OOO = BIT(1),
    CR_BLACK_OO  = BIT(2),
    CR_BLACK_OOO = BIT(3),
    CR_ALL = CR_WHITE_OO | CR_WHITE_OOO | CR_BLACK_OO | CR_BLACK_OOO
};

//
// Ranks and Files
//

enum {
    RNK_1, RNK_2, RNK_3, RNK_4,
    RNK_5, RNK_6, RNK_7, RNK_8,
    RNK_NULL
};

constexpr BoardRank RANKS[] = {
    RNK_1, RNK_2, RNK_3, RNK_4,
    RNK_5, RNK_6, RNK_7, RNK_8,
};

constexpr BoardRank RANKS_REVERSE[] = {
    RNK_8, RNK_7, RNK_6, RNK_5,
    RNK_4, RNK_3, RNK_2, RNK_1
};

#define ILLUMINA_ASSERT_VALID_RANK(r) ILLUMINA_ASSERT((r) >= illumina::RNK_1 && (r) <= illumina::RNK_8)

constexpr BoardRank rank_from_char(char c) {
    if (c > '9' || c < '0') {
        return RNK_NULL;
    }
    return c - '1';
}

constexpr Bitboard rank_bb(BoardRank rank) {
    constexpr Bitboard RANK_BBS[] {
        0xff, 0xff00,
        0xff0000, 0xff000000,
        0xff00000000, 0xff0000000000,
        0xff000000000000, 0xff00000000000000,
    };
    return RANK_BBS[rank];
}

constexpr BoardRank pawn_starting_rank(Color color) {
    constexpr BoardRank START_RANK[] = { RNK_2, RNK_7 };
    return START_RANK[color];
}

constexpr BoardRank promotion_rank(Color color) {
    constexpr BoardRank PROM_RANKS[] = { RNK_8, RNK_1 };
    return PROM_RANKS[color];
}

constexpr BoardRank double_push_dest_rank(Color color) {
    constexpr BoardRank DP_DST_RANKS[] = { RNK_4, RNK_5 };
    return DP_DST_RANKS[color];
}

enum {
    FL_A, FL_B, FL_C, FL_D,
    FL_E, FL_F, FL_G, FL_H,
    FL_NULL
};

constexpr BoardFile FILES[] = {
    FL_A, FL_B, FL_C, FL_D,
    FL_E, FL_F, FL_G, FL_H,
};

#define ILLUMINA_ASSERT_VALID_FILE(f) ILLUMINA_ASSERT((f) >= illumina::FL_A && (f) <= illumina::FL_H)

inline BoardFile file_from_char(char c) {
    c = std::tolower(c);

    if (c > 'h' || c < 'a') {
        return FL_NULL;
    }

    return c - 'a';
}

inline char file_to_char(BoardFile f) {
    ILLUMINA_ASSERT_VALID_FILE(f);

    return "abcdefgh"[f];
}

inline char rank_to_char(BoardRank r) {
    ILLUMINA_ASSERT_VALID_RANK(r);

    return '1' + r;
}

constexpr Bitboard file_bb(BoardFile file) {
    constexpr Bitboard FILE_BBS[] {
        0x101010101010101ULL,  0x202020202020202ULL,
        0x404040404040404ULL,  0x808080808080808ULL,
        0x1010101010101010ULL, 0x2020202020202020ULL,
        0x4040404040404040ULL, 0x8080808080808080ULL,
    };
    return FILE_BBS[file];
}

//
// Directions
//

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

constexpr Direction DIRECTIONS[] = {
    DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST,
    DIR_NORTHEAST, DIR_NORTHWEST,
    DIR_SOUTHEAST, DIR_SOUTHWEST
};

constexpr Direction pawn_push_direction(Color color) {
    constexpr Direction PUSH_DIRS[] = { DIR_NORTH, DIR_SOUTH };
    return PUSH_DIRS[color];
}

constexpr Direction pawn_left_capture_direction(Color color) {
    constexpr Direction LEFT_CAPT_DIRS[] = { DIR_NORTHWEST, DIR_SOUTHWEST };
    return LEFT_CAPT_DIRS[color];
}

constexpr Direction pawn_right_capture_direction(Color color) {
    constexpr Direction RIGHT_CAPT_DIRS[] = { DIR_NORTHEAST, DIR_SOUTHEAST };
    return RIGHT_CAPT_DIRS[color];
}

/**
 * Moves all the bits on a given bitboard towards the specified direction.
 *
 * Note: for convenience with double pawn pushes, DIR_NORTH*2 and DIR_SOUTH*2 can
 * be passed as an argument to D.
 */
template <Direction D>
constexpr Bitboard shift_bb(Bitboard bb) {
    switch (D) {
        case DIR_NORTH:     return bb << 8;
        case DIR_SOUTH:     return bb >> 8;
        case DIR_NORTH * 2: return bb << 16;
        case DIR_SOUTH * 2: return bb >> 16;
        case DIR_EAST:      return (bb & ~file_bb(FL_H)) << 1;
        case DIR_WEST:      return (bb & ~file_bb(FL_A)) << 1;
        case DIR_NORTHEAST: return (bb & ~file_bb(FL_H)) << 9;
        case DIR_NORTHWEST: return (bb & ~file_bb(FL_A)) << 7;
        case DIR_SOUTHEAST: return (bb & ~file_bb(FL_H)) >> 7;
        case DIR_SOUTHWEST: return (bb & ~file_bb(FL_A)) >> 9;
        default: return 0;
    }
}

//
// Squares
//

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

#define ILLUMINA_ASSERT_VALID_SQUARE(s) ILLUMINA_ASSERT((s) >= 0 && (s) <= 63)

constexpr BoardFile square_file(Square s) {
    return (s % 8);
}

constexpr BoardRank square_rank(Square s) {
    return (s / 8);
}

constexpr Square make_square(BoardFile file, BoardRank rank) {
    return rank * 8 + file;
}

constexpr Square mirror_horizontal(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

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

constexpr Square mirror_vertical(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

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
    ILLUMINA_ASSERT_VALID_SQUARE(a);
    ILLUMINA_ASSERT_VALID_SQUARE(b);

    extern int g_chebyshev[SQ_COUNT][SQ_COUNT];
    return g_chebyshev[a][b];
}

inline int manhattan_distance(Square a, Square b) {
    ILLUMINA_ASSERT_VALID_SQUARE(a);
    ILLUMINA_ASSERT_VALID_SQUARE(b);

    extern int g_manhattan[SQ_COUNT][SQ_COUNT];
    return g_manhattan[a][b];
}

inline int center_manhattan_distance(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    extern int g_center_manhattan[SQ_COUNT];
    return g_center_manhattan[s];
}

constexpr Square pawn_push_destination(Square src, Color color) {
    return src + pawn_push_direction(color);
}

constexpr Square double_push_source(Square dst, Color color) {
    return dst - pawn_push_direction(color) * 2;
}

constexpr Square double_push_destination(Square src, Color color) {
    return src + pawn_push_direction(color) * 2;
}

inline Square parse_square(std::string_view square_str) {
    BoardFile file = file_from_char(square_str[0]);
    if (file == FL_NULL) {
        return SQ_NULL;
    }

    BoardRank rank = square_str[1] - '1';
    if (rank == RNK_NULL) {
        return SQ_NULL;
    }

    return make_square(file, rank);
}

constexpr Square castled_king_square(Color c, Side side) {
    ILLUMINA_ASSERT_VALID_COLOR(c);

    constexpr Square SQUARES[] { SQ_G1, SQ_G8, SQ_C1, SQ_C8 };
    return SQUARES[side * 2 + c];
}

constexpr Square castled_rook_square(Color c, Side side) {
    ILLUMINA_ASSERT_VALID_COLOR(c);

    constexpr Square SQUARES[] { SQ_F1, SQ_F8, SQ_D1, SQ_D8 };
    return SQUARES[side * 2 + c];
}

std::string square_name(Square s);

/**
 * The square in which a eligible castling rook is expected to be in standard (non-FRC) chess
 * in the starting position, given a color and a side.
 *
 * Ex: standard_castle_rook_src_square(CL_BLACK, BS_QUEEN) = SQ_A8
 */
constexpr Square standard_castle_rook_src_square(Color color, Side side) {
    ILLUMINA_ASSERT_VALID_COLOR(color);

    constexpr Square CASTLE_ROOK_SQ[CL_COUNT][SIDE_COUNT] = {
        { SQ_H1, SQ_A1 }, // White
        { SQ_H8, SQ_A8 }  // Black
    };

    return CASTLE_ROOK_SQ[color][side];
}


//
// Bitboards
//

inline Bitboard adjacent_bb(Square s) {
    extern Bitboard g_adjacent[SQ_COUNT];
    return g_adjacent[s];
}

inline Bitboard between_bb(Square a, Square b) {
    extern Bitboard g_between[SQ_COUNT][SQ_COUNT];
    return g_between[a][b];
}

inline Bitboard between_bb_inclusive(Square a, Square b) {
    extern Bitboard g_between_inclusive[SQ_COUNT][SQ_COUNT];
    return g_between_inclusive[a][b];
}

//
// Pieces
//

enum {
    PT_NULL,
    PT_PAWN, PT_KNIGHT, PT_BISHOP,
    PT_ROOK, PT_QUEEN,  PT_KING,
    PT_COUNT
};

#define ILLUMINA_ASSERT_VALID_PIECE_TYPE(pt) ILLUMINA_ASSERT((pt) >= illumina::PT_PAWN && (pt) <= illumina::PT_KING)
#define ILLUMINA_ASSERT_VALID_PT_OR_NULL(pt) ILLUMINA_ASSERT((pt) >= illumina::PT_NULL && (pt) <= illumina::PT_KING)

constexpr PieceType PIECE_TYPES[] = {
    PT_PAWN,   PT_KNIGHT,
    PT_BISHOP, PT_ROOK,
    PT_QUEEN,  PT_KING
};

constexpr PieceType PROMOTION_PIECE_TYPES[] = {
    PT_QUEEN,  PT_ROOK,
    PT_BISHOP, PT_KNIGHT,
};

constexpr char piece_type_to_char(PieceType pt) {
    ILLUMINA_ASSERT_VALID_PT_OR_NULL(pt);

    return "-pnbrqk"[pt];
}

class Piece {
public:
    Piece() = default;

    constexpr Piece(Color color, PieceType type)
        : m_data((color & BITMASK(1)) | ((type & BITMASK(3)) << 1)) {
        ILLUMINA_ASSERT_VALID_COLOR(color);
        ILLUMINA_ASSERT_VALID_PT_OR_NULL(type);
    }

    explicit constexpr Piece(ui8 data)
        : m_data(data) {
        ILLUMINA_ASSERT_VALID_COLOR(color());
        ILLUMINA_ASSERT_VALID_PT_OR_NULL(type());
    }

    constexpr Color     color() const;
    constexpr PieceType type() const;
    constexpr ui8       raw() const;

    bool operator==(Piece other) const;
    bool operator!=(Piece other) const;

    char to_char() const;
    static Piece from_char(char c);

private:
    ui8 m_data = 0;
};

constexpr Color Piece::color() const {
    return m_data & BITMASK(1);
}

constexpr PieceType Piece::type() const {
    return (m_data >> 1);
}

constexpr ui8 Piece::raw() const {
    return m_data;
}

inline bool Piece::operator==(Piece other) const {
    return m_data == other.m_data;
}

inline bool Piece::operator!=(Piece other) const {
    return m_data != other.m_data;
}

inline std::ostream& operator<<(std::ostream& stream, Piece p) {
    stream << p.to_char();
    return stream;
}

constexpr Piece PIECE_NULL(CL_WHITE, PT_NULL);

constexpr Piece WHITE_PAWN(CL_WHITE, PT_PAWN);
constexpr Piece WHITE_KNIGHT(CL_WHITE, PT_KNIGHT);
constexpr Piece WHITE_BISHOP(CL_WHITE, PT_BISHOP);
constexpr Piece WHITE_ROOK(CL_WHITE, PT_ROOK);
constexpr Piece WHITE_QUEEN(CL_WHITE, PT_QUEEN);
constexpr Piece WHITE_KING(CL_WHITE, PT_KING);

constexpr Piece BLACK_PAWN(CL_BLACK, PT_PAWN);
constexpr Piece BLACK_KNIGHT(CL_BLACK, PT_KNIGHT);
constexpr Piece BLACK_BISHOP(CL_BLACK, PT_BISHOP);
constexpr Piece BLACK_ROOK(CL_BLACK, PT_ROOK);
constexpr Piece BLACK_QUEEN(CL_BLACK, PT_QUEEN);
constexpr Piece BLACK_KING(CL_BLACK, PT_KING);

//
// Moves
//

enum {
    MT_NORMAL,
    MT_SIMPLE_CAPTURE,
    MT_PROMOTION_CAPTURE,
    MT_EN_PASSANT,
    MT_DOUBLE_PUSH,
    MT_CASTLES,
    MT_SIMPLE_PROMOTION,
};

#define ILLUMINA_ASSERT_VALID_MOVE_TYPE(mt) ILLUMINA_ASSERT((mt) >= illumina::MT_NORMAL && (mt) <= illumina::MT_SIMPLE_PROMOTION)

class Board; // Forward declaration of Board for some Move static methods.

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
    //  26 - 28:    Castles rook file
    //  29:         Castling side
    //

public:
    Move() = default;
    Move(const Board& board, Square src, Square dst, PieceType prom_piece_type = PT_NULL);

    explicit constexpr Move(ui32 data)
        : m_data(data) {}

    constexpr ui32      raw() const;
    constexpr Square    source() const;
    constexpr Square    destination() const;
    constexpr Piece     source_piece() const;
    constexpr Piece     captured_piece() const;
    constexpr MoveType  type() const;
    constexpr PieceType promotion_piece_type() const;
    constexpr Square    castles_rook_src_square() const;
    constexpr BoardFile castles_rook_src_file() const;
    constexpr Side      castles_side() const;
    constexpr bool      is_quiet() const;
    constexpr bool      is_capture() const;
    constexpr bool      is_promotion() const;
    constexpr bool      makes_progress() const;
    std::string         to_uci(bool frc = false) const;

    bool operator==(Move other) const;
    bool operator!=(Move other) const;

protected:
    ui32 m_data;

    static constexpr Move base(Square src, Square dst, Piece src_piece, MoveType type);

public:
    static constexpr Move new_normal(Square src,
                                     Square dst,
                                     Piece src_piece);

    static constexpr Move new_simple_capture(Square src,
                                             Square dst,
                                             Piece src_piece,
                                             Piece capt_piece);

    static constexpr Move new_promotion_capture(Square src,
                                                Square dst,
                                                Color pawn_color,
                                                Piece capt_piece,
                                                PieceType prom_piece_type);

    static constexpr Move new_en_passant_capture(Square src,
                                                 Square dst,
                                                 Color pawn_color);

    static constexpr Move new_double_push(Square src, Color pawn_color);
    static constexpr Move new_double_push_from_dest(Square dst, Color pawn_color);

    static constexpr Move new_castles(Color king_color,
                                      Side side);

    static constexpr Move new_castles(Square src,
                                      Color king_color,
                                      Side side, Square
                                      rook_square);

    static constexpr Move new_simple_promotion(Square src,
                                               Square dst,
                                               Color pawn_color,
                                               PieceType prom_piece_type);

    static Move parse_uci(const Board& board, std::string_view move_str);
};

constexpr Move MOVE_NULL(0);

constexpr ui32 Move::raw() const {
    return m_data;
}

constexpr Square Move::source() const {
    return m_data & BITMASK(6);
}

constexpr Square Move::destination() const {
    return (m_data >> 6) & BITMASK(6);
}

constexpr Piece Move::source_piece() const {
    return Piece((m_data >> 12) & BITMASK(4));
}

constexpr Piece Move::captured_piece() const {
    return Piece((m_data >> 16) & BITMASK(4));
}

constexpr MoveType Move::type() const {
    return (m_data >> 20) & BITMASK(3);
}

constexpr PieceType Move::promotion_piece_type() const {
    return (m_data >> 23) & BITMASK(3);
}

constexpr BoardFile Move::castles_rook_src_file() const {
    return (m_data >> 26) & BITMASK(3);
}

constexpr Square Move::castles_rook_src_square() const {
    return make_square(castles_rook_src_file(), square_rank(source()));
}

constexpr Side Move::castles_side() const     {
    return (m_data >> 29) & BITMASK(1);
}

constexpr Move Move::base(Square src, Square dst, Piece src_piece, MoveType type) {
    ILLUMINA_ASSERT_VALID_SQUARE(src);
    ILLUMINA_ASSERT_VALID_SQUARE(dst);
    ILLUMINA_ASSERT_VALID_MOVE_TYPE(type);

    Move move(0);

    move.m_data |= (src & BITMASK(6))             << 0;
    move.m_data |= (dst & BITMASK(6))             << 6;
    move.m_data |= (src_piece.raw() & BITMASK(4)) << 12;
    move.m_data |= (type & BITMASK(3))            << 20;

    return move;
}

constexpr Move Move::new_normal(Square src,
                                Square dst,
                                Piece src_piece) {
    return base(src, dst, src_piece, MT_NORMAL);
}

constexpr Move Move::new_simple_capture(Square src,
                                        Square dst,
                                        Piece src_piece,
                                        Piece capt_piece) {
    Move move = base(src, dst, src_piece, MT_SIMPLE_CAPTURE);
    move.m_data |= (capt_piece.raw() & BITMASK(4)) << 16;
    return move;
}

constexpr Move Move::new_promotion_capture(Square src,
                                           Square dst,
                                           Color pawn_color,
                                           Piece capt_piece,
                                           PieceType prom_piece_type) {
    ILLUMINA_ASSERT_VALID_PIECE_TYPE(prom_piece_type);

    Move move = base(src, dst, Piece(pawn_color, PT_PAWN), MT_PROMOTION_CAPTURE);
    move.m_data |= (capt_piece.raw() & BITMASK(4)) << 16;
    move.m_data |= (prom_piece_type & BITMASK(3)) << 23;
    return move;
}

constexpr Move Move::new_en_passant_capture(Square src,
                                            Square dst,
                                            Color pawn_color) {
    Move move = base(src, dst, Piece(pawn_color, PT_PAWN), MT_EN_PASSANT);
    move.m_data |= (Piece(opposite_color(pawn_color), PT_PAWN).raw() & BITMASK(4)) << 16;
    return move;
}

constexpr Move Move::new_double_push(Square src, Color pawn_color) {
    Move move = base(src, double_push_destination(src, pawn_color), Piece(pawn_color, PT_PAWN), MT_DOUBLE_PUSH);
    return move;
}

constexpr Move Move::new_double_push_from_dest(Square dst, Color pawn_color) {
    Move move = base(double_push_source(dst, pawn_color), dst, Piece(pawn_color, PT_PAWN), MT_DOUBLE_PUSH);
    return move;
}

constexpr Move Move::new_castles(Color king_color,
                                 Side side) {
    return new_castles(king_color == CL_WHITE ? SQ_E1 : SQ_E8,
                       king_color, side,
                       standard_castle_rook_src_square(king_color, side));
}

constexpr Move Move::new_castles(Square src,
                                 Color king_color,
                                 Side side, Square rook_square) {
    ILLUMINA_ASSERT_VALID_SQUARE(rook_square);

    Move move = base(src, castled_king_square(king_color, side), Piece(king_color, PT_KING), MT_CASTLES);
    move.m_data |= (square_file(rook_square) & BITMASK(3)) << 26;
    move.m_data |= (side & BITMASK(1)) << 29;
    return move;
}

constexpr Move Move::new_simple_promotion(Square src,
                                          Square dst,
                                          Color pawn_color,
                                          PieceType prom_piece_type) {
    ILLUMINA_ASSERT_VALID_PIECE_TYPE(prom_piece_type);

    Move move = base(src, dst, Piece(pawn_color, PT_PAWN), MT_SIMPLE_PROMOTION);
    move.m_data |= (prom_piece_type & BITMASK(3)) << 23;
    return move;
}

constexpr bool Move::is_capture() const {
    constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE) | BIT(MT_EN_PASSANT) | BIT(MT_PROMOTION_CAPTURE);
    return (BIT(type()) & MASK) != 0;
}

constexpr bool Move::is_promotion() const {
    constexpr ui64 MASK = BIT(MT_PROMOTION_CAPTURE) | BIT(MT_SIMPLE_PROMOTION);
    return (BIT(type()) & MASK) != 0;
}

constexpr bool Move::is_quiet() const {
    constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE) | BIT(MT_EN_PASSANT) | BIT(MT_PROMOTION_CAPTURE) | BIT(MT_SIMPLE_PROMOTION);
    return (BIT(type()) & MASK) == 0;
}

constexpr bool Move::makes_progress() const {
    return is_capture() || source_piece().type() == PT_PAWN;
}

inline bool Move::operator==(Move other) const {
    return m_data == other.m_data;
}

inline bool Move::operator!=(Move other) const {
    return m_data != other.m_data;
}

inline std::ostream& operator<<(std::ostream& stream, Move move) {
    stream << move.to_uci() << std::hex << " (0x" << ui32(move.raw()) << ")" << std::dec;
    return stream;
}

} // illumina

#endif // ILLUMINA_TYPES_H
