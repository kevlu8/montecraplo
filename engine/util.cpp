#include "util.hpp"

bool is_game_over(Board &board) {
    if (_mm_popcnt_u64(board.piece_boards[KING]) < 2) {
        // There are less than 2 kings thus the game is over
        return true;
    }

    if (board.threefold() || board.halfmove >= 100) {
        return true;
    }

	return false;
}