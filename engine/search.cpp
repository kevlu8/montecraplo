#include "search.hpp"
#include <algorithm>

fast_random rng(1);
uint64_t its = 0;

void clear_nodes(MCTSNode *root) {
    for (auto &child : root->children) {
        clear_nodes(child);
        delete child;
    }
    root->children.clear();
}

int to_cp_eval(int visits, int val) {
    if (visits == 0) return 0;
    return ((double)val / visits) * 10000; // +100 = definite win, -100 = definite loss
}

// Phase 1: Selection
// Iterate DFS-style through the tree, choosing the child with maximum UCB1
// until we hit a leaf node (no children). We select and return this leaf node.
MCTSNode *select(MCTSNode *u) {
    if (u->leaf) return u;
    double best_ucb1 = -INFINITY;
    MCTSNode *best_child = nullptr;

    for (auto v : u->children) {
        double ucb1 = v->ucb1();
        if (ucb1 > best_ucb1) {
            best_ucb1 = ucb1;
            best_child = v;
        }
    }

    if (!best_child) return u;
    return select(best_child);
}

// Phase 2: Expansion
// Take the selected node and expand its children (i.e. do movegen).
// Pick a random child and return it for rollout.
MCTSNode *expand(MCTSNode *u) {
    if (!u->leaf) return u; // Already expanded
    u->leaf = false;

    pzstd::vector<Move> moves;
    u->pos.pseudolegal_moves(moves);

    for (auto &m : moves) {
        if (!u->pos.is_legal(m)) continue;

        MCTSNode *c = new MCTSNode();
        c->parent = u;
        c->move = m;
        c->pos = u->pos;
        c->pos.make_move(m);

        u->children.push_back(c);
    }

    return u->children.empty() ? u : u->children[rng.next() % u->children.size()];
}

// Phase 3: Simulation / Rollout
// Take the selected child and simulate a random game. Return the result.
int rollout(MCTSNode *u) {
    Position pos = u->pos; // Need to copy
    int res = 0;
    while (true) {
        // Check for game over (kinda expensive)
        // To do this easily, we can do a movegen and check is_legal() on
        // each move. If no moves are legal, the game is over.
        
        // First, check the cheaper stuff
        if (pos.halfmove >= 100 || pos.insufficient_material()) break;

        // Where is threefold? Well, there's no cheap way of checking it.
        // We can't just copy the entire position history...
        // For now, rely on the other draw conditions

        // Now for mate detection
        pzstd::vector<Move> moves, legal_moves;
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
                // if u is also white, then the result of this rollout is
                // a loss for u, so we set res = -1.
                res = pos.side == u->pos.side ? -1 : 1;
            else
                res = 0; // Stalemate
            break;
        }

        // Pick a random move
        Move m = legal_moves[rng.next() % legal_moves.size()];
        pos.make_move(m);
    }

    return res;
}

// Phase 4: Backpropagation
// Take the result of the game and send it back up the tree.
void backprop(MCTSNode *u, int res) {
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

    for (auto u : root->children) {
        if (u->visits > best_visits) {
            best_visits = u->visits;
            best_child = u;
        }
    }

    return best_child;
}

void search(Position &pos, int time) {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    MCTSNode *root = new MCTSNode();
    root->pos = pos;

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
            if (total_nodes * 3000 / 1024 / 1024 >= 256) break;
        }

        auto *u = select(root); // Select a leaf node
        auto *c = expand(u); // Expand the leaf node and get the child
        int res = rollout(c); // Simulate a game and get the result
        backprop(c, res); // Propagate the result
    }

    std::cout << "info string visits:\n";
    uint64_t tot = root->visits;
    for (auto u : root->children) {
        std::cout << "info string " << u->move.to_string() << ": " << u->visits * 100 / tot << "% = " << u->visits << "\n";
    }
    std::cout << "bestmove " << bestchild(root)->move.to_string() << std::endl;

    clear_nodes(root);
    total_nodes = 0;
}
