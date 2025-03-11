#include "marlinflow.h"

#include <illumina.h>
#include <string>
#include <unordered_map>
#include <sstream>

namespace illumina {

static std::unordered_map<BoardOutcome, std::array<std::string, CL_COUNT>> s_wdl_map = {
    { BoardOutcome::UNFINISHED, { "0.5", "0.5" } },
    { BoardOutcome::STALEMATE, { "0.5", "0.5" } },
    { BoardOutcome::CHECKMATE, { "1.0", "0.0" } },
    { BoardOutcome::DRAW_BY_REPETITION, { "0.5", "0.5" } },
    { BoardOutcome::DRAW_BY_50_MOVES_RULE, { "0.5", "0.5" } },
    { BoardOutcome::DRAW_BY_INSUFFICIENT_MATERIAL, { "0.5", "0.5" } },
};

ui64 MarlinflowDataWriter::write_data(ThreadContext& ctx,
                                      std::ostream& stream,
                                      const Game& game,
                                      const std::vector<ExtractedData>& extracted_data) {
    ui64 data_points = 0;
    for (const ExtractedData& data: extracted_data) {
        std::stringstream ss;
        stream << data.fen                      << " | "
               << data.ply_data.white_pov_score << " | "
               << s_wdl_map.at(game.result.outcome)[game.result.winner.value_or(CL_WHITE)];

        std::string s = ss.str();
        stream << s;
        data_points++;
    }
    return data_points;
}

MarlinflowDataWriter::~MarlinflowDataWriter() noexcept { }

} // illumina
