#include "search.hpp"
#include <algorithm>

fast_random rng(1);
uint64_t its = 0;

void clear_nodes(MCTSNode *u) {
	if (!u) return;
	MCTSNode *cur = u->first_child;
	while (cur) {
		MCTSNode *next = cur->next_sibling;
		clear_nodes(cur);
		delete cur;
		cur = next;
	}
}

// Phase 1: Selection
// Iterate DFS-style through the tree, choosing the child with maximum UCB1
// until we hit a leaf node (no children). We select and return this leaf node.
MCTSNode *select(MCTSNode *u, Position &pos, RepetitionHandler &rp, AccumulatorManager &am) {
	if (!u->first_child) return u; // May return a terminal node
	double best_puct = -INFINITY;
	MCTSNode *best_child = nullptr;

	// Query policy head for move scores
	std::array<float, PHEAD_SIZE> policy;
	if (pos.side == WHITE) policy = nn_policy(nn, am.current().w_acc, am.current().b_acc);
	else policy = nn_policy(nn, am.current().b_acc, am.current().w_acc);

	// Mask out illegal moves
	bool legal[PHEAD_SIZE] = {};
	MCTSNode *cur = u->first_child;
	while (cur) {
		Move m = cur->move;
		legal[move_to_policy(m)] = true;
		cur = cur->next_sibling;
	}

	double total = 0;
	for (int i = 0; i < PHEAD_SIZE; i++) {
		if (legal[i]) total += std::exp(policy[i]); // softmax total
	}

	cur = u->first_child;
	while (cur) {
		double prob = std::exp(policy[move_to_policy(cur->move)]) / total;
		double puct = cur->puct(prob);
		if (puct > best_puct) {
			best_puct = puct;
			best_child = cur;
		}
		cur = cur->next_sibling;
	}

	pos.make_move(best_child->move);
	rp.push_hash(pos.zobrist_without_ep());
	am.make_move(pos, best_child->move);
	return select(best_child, pos, rp, am);
}

// Phase 2: Expansion
// Take the selected node and expand its children (i.e. do movegen).
void expand(MCTSNode *u, Position &pos, RepetitionHandler &rp) {
	if (u->first_child) return; // Already expanded
	if (u->terminal) return; // Already terminal

	// Check for game-over states
	if (pos.halfmove >= 100 || pos.insufficient_material() || rp.threefold(0, pos.zobrist_without_ep())) {
		u->terminal = true;
		return;
	}

	pzstd::vector<Move> moves;
	pos.pseudolegal_moves(moves);

	MCTSNode *prev = nullptr;

	for (auto &m : moves) {
		if (!pos.is_legal(m)) continue;

		MCTSNode *c = new MCTSNode();
		c->parent = u;
		c->move = m;

		// Link the new node to the parent or its previous sibling
		if (!u->first_child) {
			u->first_child = c;
		} else {
			prev->next_sibling = c;
		}
		prev = c;
	}

	if (!u->first_child) {
		// No children means end of game (checkmate or stalemate)
		u->terminal = true;
	}
}

// Phase 3: Simulation / Rollout
// Take the selected child and simulate a random game. Return the result.
// In modern MCTS, we do one deep NN inference instead of actually playing a game.
double rollout(MCTSNode *u, Position &pos, AccumulatorManager &am) {
	return eval(pos, am);
}

// Phase 4: Backpropagation
// Take the result of the game and send it back up the tree.
void backprop(MCTSNode *u, double res) {
	while (u) {
		u->visits++;
		res *= -1;
		u->val += res;
		u = u->parent;
	}
}

// Fetching the best child
// In MCTS, we generally select the child with the most visits as
// the best one.
MCTSNode *bestchild(MCTSNode *root) {
	int best_visits = 0;
	MCTSNode *best_child = nullptr;

	MCTSNode *cur = root->first_child;
	while (cur) {
		if (cur->visits > best_visits) {
			best_visits = cur->visits;
			best_child = cur;
		}
		cur = cur->next_sibling;
	}

	return best_child;
}

void print_pv(MCTSNode *root) {
	MCTSNode *cur = root;
	while (cur) {
		MCTSNode *best_child = bestchild(cur);
		if (best_child) {
			std::cout << best_child->move.to_string() << " ";
		}
		cur = best_child;
	}
}

Move search(Position &p, RepetitionHandler &rp, AccumulatorManager &am, int time, uint64_t visits, void *opt_visdistr) {
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	MCTSNode *root = new MCTSNode();

	its = 0;
	while (its < visits) {
		its++;
		if ((its & 127) == 0) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() + 1;

			// if ((its & 1023) == 0) {
			// 	// Print info
			// 	MCTSNode *best_child = bestchild(root);
			// 	std::cout << "info depth 1 score cp " << int(best_child->val * 100 / best_child->visits) << " nodes " << its << " winrate " << best_child->val / best_child->visits << " mctsnodes " << total_nodes
			// 			<< " time " << elapsed << " nps " << its * 1000 / elapsed << " pv ";
			// 	print_pv(root);
			// 	std::cout << std::endl;
			// }

			// Check for time limit
			if (elapsed >= time) break;

			// Check for mem limit
			if (total_nodes * sizeof(MCTSNode) / 1024 / 1024 >= 256) break;
		}

		Position pos = p; // Must copy to avoid modifying the original
		RepetitionHandler rp_copy = rp;
		AccumulatorManager am_copy = am;

		auto *u = select(root, pos, rp_copy, am_copy); // Select a leaf node
		expand(u, pos, rp_copy); // Expand the leaf node and get the child
		double res = rollout(u, pos, am_copy); // Simulate a game and get the result
		backprop(u, res); // Propagate the result
	}

	MCTSNode *best_child = bestchild(root);
	// auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() + 1;
	// std::cout << "info depth 1 score cp " << int(best_child->val * 100 / best_child->visits) << " nodes " << its << " winrate " << best_child->val / best_child->visits << " mctsnodes " << total_nodes
	// 		<< " time " << elapsed << " nps " << its * 1000 / elapsed << " pv ";
	// print_pv(root);
	// std::cout << std::endl;

	// std::cout << "info string visits:\n";
	uint64_t tot = root->visits;
	// MCTSNode *cur = root->first_child;
	// while (cur) {
	// 	std::cout << "info string " << cur->move.to_string() << ": " << cur->visits * 100 / tot << "% = " << cur->visits << "\n";
	// 	cur = cur->next_sibling;
	// }
	// std::cout << "bestmove " << best_child->move.to_string() << std::endl;

	if (opt_visdistr) {
		MCTSNode *cur = root->first_child;
		while (cur) {
			std::array<uint64_t, PHEAD_SIZE> *visits = (std::array<uint64_t, PHEAD_SIZE>*)opt_visdistr;
			(*visits)[move_to_policy(cur->move)] = cur->visits / (double)tot;
			cur = cur->next_sibling;
		}
	}

	Move move = best_child->move;

	clear_nodes(root);
	total_nodes = 0;
	return move;
}
