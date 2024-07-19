#include "illumina.h"

#include <atomic>

namespace illumina {

static std::atomic_bool s_initialized = false;

void init_attacks();
void init_types();
void init_zob();
void init_search();

void init() {
    if (s_initialized) {
        return;
    }

    s_initialized = true;

    init_eval();
    init_types();
    init_zob();
    init_attacks();
    init_search();
}

bool initialized() {
    return s_initialized;
}

} // illumina