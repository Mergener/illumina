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

void MarlinflowDataWriter::write_data(ThreadContext& ctx,
                                      std::ostream& stream,
                                      const Game& game,
                                      const std::vector<ExtractedData>& extracted_data) {
    for (const ExtractedData& data: extracted_data) {
        std::stringstream ss;
        stream << data.fen                      << " | "
               << data.ply_data.white_pov_score << " | "
               << s_wdl_map.at(game.result.outcome)[game.result.winner.value_or(CL_WHITE)];

        std::string s = ss.str();
        ctx.bytes_written += s.size();
        stream << s;
        ctx.positions_written++;

        if ((ctx.positions_written - 1) % 1000 == 999) {
            TimePoint now = Clock::now();

            double dt = double(delta_ms(now, ctx.time_start)) / 1000.0;
            double pos_per_sec = double(ctx.positions_written) / dt;

            stream.flush();
            std::cout << ctx.positions_written << " positions generated so far. ("
                      << int(dt) << " sec"
                      << ", " << int(pos_per_sec) << " pos/sec"
                      << ", " << int(ctx.bytes_written) << " bytes"
                      << ", " << int(double(ctx.bytes_written) / double(ctx.positions_written)) << " bytes/pos"
                      << ")" << std::endl;

        }
    }
}

MarlinflowDataWriter::~MarlinflowDataWriter() noexcept { }

} // illumina
