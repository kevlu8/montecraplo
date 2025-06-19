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
	return std::min(std::max((double)score / 1000, -1.0), 1.0);
}

double fast_eval(Board &board) {
    double white_material = 0, black_material = 0;
    
    white_material += _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(WHITE)]) * QueenValue;
    white_material += _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(WHITE)]) * RookValue;
    white_material += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]) * BishopValue;
    white_material += _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(WHITE)]) * KnightValue;
    white_material += _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)]) * PawnValue;
    
    black_material += _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(BLACK)]) * QueenValue;
    black_material += _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(BLACK)]) * RookValue;
    black_material += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]) * BishopValue;
    black_material += _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(BLACK)]) * KnightValue;
    black_material += _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)]) * PawnValue;
    
    double material_diff = white_material - black_material;

    if (material_diff >= 500) return 0.95;
    if (material_diff <= -500) return -0.95;
    if (material_diff >= 300) return 0.8;
    if (material_diff <= -300) return -0.8;

    return std::min(std::max(material_diff / 250.0, -0.7), 0.7);
}