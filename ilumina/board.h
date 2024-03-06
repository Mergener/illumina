#ifndef ILUMINA_BOARD_H
#define ILUMINA_BOARD_H

namespace ilumina {

class Board {
public:

    Board() = default;

    ~Board() = default;

    Board(Board&& rhs) = default;

    Board& operator=(const Board& rhs) = default;

private:

};

} // ilumina

#endif // ILUMINA_BOARD_H
