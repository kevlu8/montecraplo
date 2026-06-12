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
MCTSNode *select(MCTSNode *u, Position &pos, RepetitionHandler &rp) {
	if (!u->first_child) return u; // May return a terminal node
	double best_ucb1 = -INFINITY;
	MCTSNode *best_child = nullptr;

	MCTSNode *cur = u->first_child;
	while (cur) {
		double ucb1 = cur->ucb1();
		if (ucb1 > best_ucb1) {
			best_ucb1 = ucb1;
			best_child = cur;
		}
		cur = cur->next_sibling;
	}

	if (!best_child) return u;
	pos.make_move(best_child->move);
	rp.push_hash(pos.zobrist_without_ep());
	return select(best_child, pos, rp);
}

// Phase 2: Expansion
// Take the selected node and expand its children (i.e. do movegen).
// Pick a random child and return it for rollout.
MCTSNode *expand(MCTSNode *u, Position &pos, RepetitionHandler &rp) {
	if (u->first_child) return u; // Already expanded
	if (u->terminal) return u; // Already terminal

	// Check for game-over states
	if (pos.halfmove >= 100 || pos.insufficient_material() || rp.threefold(0, pos.zobrist_without_ep())) {
		u->terminal = true;
		return u;
	}

	pzstd::vector<Move> moves;
	pos.pseudolegal_moves(moves);

	pzstd::vector<MCTSNode *> children;

	for (auto &m : moves) {
		if (!pos.is_legal(m)) continue;

		MCTSNode *c = new MCTSNode();
		c->parent = u;
		c->move = m;

		// Link the new node to the parent or its previous sibling
		if (!u->first_child) {
			u->first_child = c;
		} else {
			MCTSNode *current = children[children.size() - 1];
			current->next_sibling = c;
		}
		children.push_back(c);
	}

	if (children.empty()) {
		// No children means end of game (checkmate or stalemate)
		u->terminal = true;
		return u;
	}

	return children[rng.next() % children.size()];
}

// Phase 3: Simulation / Rollout
// Take the selected child and simulate a random game. Return the result.
double rollout(MCTSNode *u, Position &p, RepetitionHandler &rp) {
	Position pos = p; // Must copy to avoid modifying the original
	int ply = 0;
	double res = 0.0;
	pzstd::vector<Move> moves, legal_moves;
	while (true) {
		// First, check for excessively long games. Once our rollout reaches,
		// say, 40 plies, we stop the rollout and return a simple evaluation.
		if (ply >= 40) {
			res = std::clamp(eval(pos) / 1000.0, -1.0, 1.0) * (pos.side == p.side ? 1 : -1);
			break;
		}

		// Check for game over (kinda expensive)
		// To do this easily, we can do a movegen and check is_legal() on
		// each move. If no moves are legal, the game is over.
		
		// First, check the cheaper stuff
		if (pos.halfmove >= 100 || pos.insufficient_material() || rp.threefold(ply, pos.zobrist_without_ep())) break;

		// Now for mate detection
		moves.clear(); legal_moves.clear();
		pos.pseudolegal_moves(moves);
		bool legal_exists = false;
		for (const auto &m : moves) {
			if (pos.is_legal(m)) {
				legal_exists = true;
				legal_moves.push_back(m);
			}
		}

		if (!legal_exists) {
			if (pos.checkers[pos.side])
				// imagine pos.side == white, this means white lost.
				// if p.side is also white, then the result of this rollout is
				// a loss for u, so we set res = -1.
				res = pos.side == p.side ? -1 : 1;
			else
				res = 0; // Stalemate
			break;
		}

		// Pick a random move
		Move m = legal_moves[rng.next() % legal_moves.size()];
		pos.make_move(m);
		rp.push_hash(pos.zobrist_without_ep());
		ply++;
	}

	return res;
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

void search(Position &p, RepetitionHandler &rp, int time) {
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	MCTSNode *root = new MCTSNode();

	its = 0;
	while (true) {
		its++;
		if ((its & 127) == 0) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

			if ((its & 1023) == 0) {
				// Print info
				MCTSNode *best_child = bestchild(root);
				std::cout << "info depth 1 score cp " << int(best_child->val * 100 / best_child->visits) << " nodes " << its << " winrate " << best_child->val / best_child->visits << " mctsnodes " << total_nodes
						<< " time " << elapsed << " nps " << its * 1000 / elapsed << " pv " << best_child->move.to_string() << std::endl;
			}

			// Check for time limit
			if (elapsed >= time) break;

			// Check for mem limit
			if (total_nodes * sizeof(MCTSNode) / 1024 / 1024 >= 256) break;
		}

		Position pos = p; // Must copy to avoid modifying the original
		RepetitionHandler rp_copy = rp;

		auto *u = select(root, pos, rp_copy); // Select a leaf node
		auto *c = expand(u, pos, rp_copy); // Expand the leaf node and get the child

		if (!u->terminal) {
			pos.make_move(c->move);
			rp_copy.push_hash(pos.zobrist_without_ep());
		}

		double res = rollout(c, pos, rp_copy); // Simulate a game and get the result
		backprop(c, res); // Propagate the result
	}

	MCTSNode *best_child = bestchild(root);
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	std::cout << "info depth 1 score cp " << int(best_child->val * 100 / best_child->visits) << " nodes " << its << " winrate " << best_child->val / best_child->visits << " mctsnodes " << total_nodes
			<< " time " << elapsed << " nps " << its * 1000 / elapsed << " pv " << best_child->move.to_string() << std::endl;

	std::cout << "info string visits:\n";
	uint64_t tot = root->visits;
	MCTSNode *cur = root->first_child;
	while (cur) {
		std::cout << "info string " << cur->move.to_string() << ": " << cur->visits * 100 / tot << "% = " << cur->visits << "\n";
		cur = cur->next_sibling;
	}
	std::cout << "bestmove " << best_child->move.to_string() << std::endl;

	clear_nodes(root);
	total_nodes = 0;
}
