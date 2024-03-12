#include "zobrist.h"

#include <ctime>

namespace illumina {

ui64 g_piece_square_keys[PT_COUNT][CL_COUNT][64];
ui64 g_castling_rights_keys[16];
ui64 g_color_to_move_keys[2];
ui64 g_en_passant_square_keys[256];

static struct {
    ui8 a = 166;
    ui8 b = 124;
    ui8 c = 13;
    ui8 d = 249;
} s_random_context;

static ui8 random_ui8() {
    ui8 e = s_random_context.a - lrot(s_random_context.b, 7);

    s_random_context.a = s_random_context.b ^ lrot(s_random_context.c, 13);
    s_random_context.b = s_random_context.c + lrot(s_random_context.d, 37);
    s_random_context.c = s_random_context.d + e;
    s_random_context.d = e + s_random_context.a;

    return s_random_context.d;
}

static ui64 random_ui64() {
    return (ui64(random_ui8()) << 56) |
           (ui64(random_ui8()) << 48) |
           (ui64(random_ui8()) << 40) |
           (ui64(random_ui8()) << 32) |
           (ui64(random_ui8()) << 24) |
           (ui64(random_ui8()) << 16) |
           (ui64(random_ui8()) << 8) |
           (ui64(random_ui8()));
}

static void fill_keys(ui64* it, size_t size) {
    ui64* last = it + size / sizeof(ui64);

    while (it != last) {
        *it = random_ui64();

        it++;
    }
}

void init_zob() {
    // Initialize piece square keys
    fill_keys(reinterpret_cast<ui64*>(g_piece_square_keys), sizeof(g_piece_square_keys));
    fill_keys(g_castling_rights_keys, sizeof(g_castling_rights_keys));
    fill_keys(g_color_to_move_keys, sizeof(g_color_to_move_keys));
    fill_keys(g_en_passant_square_keys, sizeof(g_en_passant_square_keys));
}

} // illumina
