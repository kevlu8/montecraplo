#include "accumulator.hpp"

void AccumulatorManager::AccumulatorPair::update_add(Square sq, PieceType pt, bool side) {
	uint16_t w_index = calculate_index(sq, pt, side, 0);
	uint16_t b_index = calculate_index(sq, pt, side, 1);
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] += nn.accumulator_weights[w_index][i];
		b_acc.val[i] += nn.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_sub(Square sq, PieceType pt, bool side) {
	uint16_t w_index = calculate_index(sq, pt, side, 0);
	uint16_t b_index = calculate_index(sq, pt, side, 1);
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] -= nn.accumulator_weights[w_index][i];
		b_acc.val[i] -= nn.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::full_refresh(Position &pos, int index) {
	// Init the first accumulator so we have a basepoint
	for (int i = 0; i < L1_SIZE; i++) {
		accs[index].w_acc.val[i] = nn.accumulator_biases[i];
		accs[index].b_acc.val[i] = nn.accumulator_biases[i];
	}

	Square wkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			accs[index].update_add((Square)i, pt, side);
		}
	}

	accs[index].correct = true;
}

void AccumulatorManager::apply_lazy(Position &pos) {
	full_refresh(pos, idx);
	return;
	if (current().correct) return; // No updates needed
	int index = idx;
	bool good_found = false;
	while (true) {
		index--;

		if (accs[index].correct) {
			// Found a basepoint we can do incremental off of
			// Note that it is implied that the buckets are the same and have not changed
			good_found = true;
			break;
		}
	}

	if (!good_found) {
		// :(
		full_refresh(pos, idx);
		return;
	}

	for (int i = index + 1; i <= idx; i++) {
		auto &u = updates[i];
		if (u.deltas == 2) {
			// -+
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nn.accumulator_weights[u.w_deltas[0]][k] + nn.accumulator_weights[u.w_deltas[1]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nn.accumulator_weights[u.b_deltas[0]][k] + nn.accumulator_weights[u.b_deltas[1]][k];
			}
		} else if (u.deltas == 3) {
			// --+
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nn.accumulator_weights[u.w_deltas[0]][k] - nn.accumulator_weights[u.w_deltas[1]][k] + nn.accumulator_weights[u.w_deltas[2]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nn.accumulator_weights[u.b_deltas[0]][k] - nn.accumulator_weights[u.b_deltas[1]][k] + nn.accumulator_weights[u.b_deltas[2]][k];
			}
		} else if (u.deltas == 4) {
			// --++
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nn.accumulator_weights[u.w_deltas[0]][k] - nn.accumulator_weights[u.w_deltas[1]][k] + nn.accumulator_weights[u.w_deltas[2]][k] + nn.accumulator_weights[u.w_deltas[3]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nn.accumulator_weights[u.b_deltas[0]][k] - nn.accumulator_weights[u.b_deltas[1]][k] + nn.accumulator_weights[u.b_deltas[2]][k] + nn.accumulator_weights[u.b_deltas[3]][k];
			}
		}
		accs[i].correct = true;
	}
}

void AccumulatorManager::make_move(Position &pos, Move move) {
	idx++;
	AccumulatorPair &acc = accs[idx], &prev_acc = accs[idx - 1];
	acc.correct = false;

	// 5 cases: quiet, promo, capture, en passant, castling
	bool promo = move.type() == PROMOTION;
	bool capture = pos.is_capture(move);
	bool ep = move.type() == EN_PASSANT;
	bool castle = move.type() == CASTLING;

	if (castle) {
		Square king_dest, rook_dest;
		if (pos.side == WHITE) {
			king_dest = move.src() < move.dst() ? SQ_G1 : SQ_C1;
			rook_dest = move.src() < move.dst() ? SQ_F1 : SQ_D1;
		} else {
			king_dest = move.src() < move.dst() ? SQ_G8 : SQ_C8;
			rook_dest = move.src() < move.dst() ? SQ_F8 : SQ_D8;
		}
		// 4 updates: rm king, rm rook, add king, add rook
		int windex1 = calculate_index(move.src(), KING, pos.side, 0);
		int bindex1 = calculate_index(move.src(), KING, pos.side, 1);
		int windex2 = calculate_index(move.dst(), ROOK, pos.side, 0);
		int bindex2 = calculate_index(move.dst(), ROOK, pos.side, 1);
		int windex3 = calculate_index(king_dest, KING, pos.side, 0);
		int bindex3 = calculate_index(king_dest, KING, pos.side, 1);
		int windex4 = calculate_index(rook_dest, ROOK, pos.side, 0);
		int bindex4 = calculate_index(rook_dest, ROOK, pos.side, 1);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3, windex4, bindex4};
		return;
	}

	if (ep) {
		// 3 updates: rm pawn, rm taken pawn, add pawn
		Square taken_pawn = Square((move.src() & 0b111000) | (move.dst() & 0b000111));
		int windex1 = calculate_index(move.src(), PAWN, pos.side, 0);
		int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1);
		int windex2 = calculate_index(taken_pawn, PAWN, !pos.side, 0);
		int bindex2 = calculate_index(taken_pawn, PAWN, !pos.side, 1);
		int windex3 = calculate_index(move.dst(), PAWN, pos.side, 0);
		int bindex3 = calculate_index(move.dst(), PAWN, pos.side, 1);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	if (promo) {
		if (!capture) {
			// 2 updates: rm pawn, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1);
			int windex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0);
			int bindex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1);
			updates[idx] = {windex1, bindex1, windex2, bindex2};
		} else {
			// 3 updates: rm pawn, rm captured piece, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1);
			PieceType captured_pt = PieceType(pos.mailbox[move.dst()] & 7);
			int windex2 = calculate_index(move.dst(), captured_pt, !pos.side, 0);
			int bindex2 = calculate_index(move.dst(), captured_pt, !pos.side, 1);
			int windex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0);
			int bindex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1);
			updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		}
		return;
	}

	if (capture) {
		// 3 updates: rm piece, rm captured, add piece
		int windex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0);
		int bindex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1);
		int windex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.dst()] & 7), !pos.side, 0);
		int bindex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.dst()] & 7), !pos.side, 1);
		int windex3 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0);
		int bindex3 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	// 2 updates: rm piece, add piece
	int windex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0);
	int bindex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1);
	int windex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0);
	int bindex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1);
	updates[idx] = {windex1, bindex1, windex2, bindex2};
}
