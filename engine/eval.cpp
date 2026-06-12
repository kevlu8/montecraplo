#include "eval.hpp"

Value eval(const Position &pos) {
	return 100 * arch::popcnt(pos.piece_boards[PAWN] & pos.piece_boards[OCC(pos.side)])
		+ 300 * arch::popcnt(pos.piece_boards[KNIGHT] & pos.piece_boards[OCC(pos.side)])
		+ 300 * arch::popcnt(pos.piece_boards[BISHOP] & pos.piece_boards[OCC(pos.side)])
		+ 500 * arch::popcnt(pos.piece_boards[ROOK] & pos.piece_boards[OCC(pos.side)])
		+ 900 * arch::popcnt(pos.piece_boards[QUEEN] & pos.piece_boards[OCC(pos.side)])
		- 100 * arch::popcnt(pos.piece_boards[PAWN] & pos.piece_boards[OCC(!pos.side)])
		- 300 * arch::popcnt(pos.piece_boards[KNIGHT] & pos.piece_boards[OCC(!pos.side)])
		- 300 * arch::popcnt(pos.piece_boards[BISHOP] & pos.piece_boards[OCC(!pos.side)])
		- 500 * arch::popcnt(pos.piece_boards[ROOK] & pos.piece_boards[OCC(!pos.side)])
		- 900 * arch::popcnt(pos.piece_boards[QUEEN] & pos.piece_boards[OCC(!pos.side)]);
}