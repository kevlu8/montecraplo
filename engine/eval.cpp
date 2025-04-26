#include "eval.hpp"

double eval(Board &board) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += _mm_popcnt_u64(board.piece_boards[i] | board.piece_boards[OCC(WHITE)]) * PieceValue[i];
		score -= _mm_popcnt_u64(board.piece_boards[i] | board.piece_boards[OCC(BLACK)]) * PieceValue[i];
	}
	// Bind score to [-1, 1]
	// Let's say that 10000 is a win for white and -10000 is a win for black
	return std::min(1.0, std::max(-1.0, (double)score / 10000));
}