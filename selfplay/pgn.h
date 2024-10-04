#ifndef ILLUMINA_PGN_H
#define ILLUMINA_PGN_H

#include <string>
#include <map>
#include <memory>
#include <string_view>

#include "board.h"

namespace illumina {

struct PGNNode {
    Move move;
    std::vector<PGNNode> variation;
    std::string annotation;
};

struct PGNGame {
    std::vector<PGNNode> main_line;
    std::map<std::string, std::string> tags;
    BoardResult result;

    void load_from(std::istream& stream);
};

class PGN {
public:
    PGNGame& push_game();
    PGNGame pop_game();

    std::string serialize() const;
    void serialize(std::ostream& stream) const;
    void deserialize(std::string_view str, bool append = true);
    void deserialize(std::istream& stream, bool append = true);

    explicit PGN(std::istream& stream);
    PGN(std::string_view str);
    PGN() = default;

private:
    std::vector<PGNGame> m_games;
};

} // illumina

#endif // ILLUMINA_PGN_H
