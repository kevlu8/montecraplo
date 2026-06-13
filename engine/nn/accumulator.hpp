#pragma once

#include "../includes.hpp"
#include "network.hpp"
#include "../bitboard.hpp"

struct AccumulatorManager {
	struct AccumulatorPair {
		Accumulator w_acc, b_acc;
		bool correct = false;

		void update_add(Square sq, PieceType pt, bool side);
		void update_sub(Square sq, PieceType pt, bool side);
	};

	struct Update {
		int w_deltas[4], b_deltas[4];
		int deltas = 0;

		Update() : deltas(0) {}
		Update(int w1, int b1, int w2, int b2) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; deltas = 2; }
		Update(int w1, int b1, int w2, int b2, int w3, int b3) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; deltas = 3; }
		Update(int w1, int b1, int w2, int b2, int w3, int b3, int w4, int b4) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; w_deltas[3] = w4; b_deltas[3] = b4; deltas = 4; }
	};

	std::array<AccumulatorPair, MAX_PLY + 5> accs = {};
	int idx = 0;
	std::array<Update, MAX_PLY + 5> updates = {}; // Stores the changed indices for each move - updates[i] stores the changes from accs[i-1] to accs[i]

	AccumulatorManager() {}

	AccumulatorManager(Position &pos) {
		full_refresh(pos, 0);
	}

	AccumulatorPair &current() {
		return accs[idx];
	}

	/**
	 * Recomputes the accumulator at index i from scratch based on the given position.
	 */
	void full_refresh(Position &pos, int index);

	/**
	 * Updates the accumulator stack
	 */
	void apply_lazy(Position &pos);

	/**
	 * Updates the accumulator stack based on the move. `pos_after` is only used if the king crosses
	 * a boundary, in which case we do a full refresh. Otherwise, we just do an incremental update.
	 */
	void make_move(Position &pos, Move move);

	/**
	 * Reverts previously made move, restoring the previous accumulator. There are no safeguards to
	 * prevent popping past the beginning.
	 */
	void pop_move() {
		accs[idx].correct = false;
		idx--;
	}
};