#include "eval.hpp"

Network nn_network;

double eval(Board &board) {
	Accumulator w_acc, b_acc;
	// Query the NNUE network
	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		
		accumulator_add(nn_network, w_acc, calculate_index((Square)i, pt, side, true));
		accumulator_sub(nn_network, b_acc, calculate_index((Square)i, pt, side, false));
	}

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	int nbucket = (npieces - 2) / 4;

	int32_t score;
	if (board.side == WHITE) {
		score = nn_eval(nn_network, w_acc, b_acc, nbucket);
	} else {
		score = -nn_eval(nn_network, b_acc, w_acc, nbucket);
	}
	
	// Bind score to [-1, 1]
	return std::min(std::max((double)score / 10000, -1.0), 1.0);
}