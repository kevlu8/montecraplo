#include "search.hpp"

int games = 0, limit = 500000, last_check = 0;
clock_t start;
int max_time = 0;

void clear_nodes(MCTSNode *node) {
    for (auto &child : node->children) {
        clear_nodes(child);
        delete child;
    }
    node->children.clear();
}

int to_cp_eval(int nsims, int val) {
    if (nsims == 0) return 0;
    return ((double)val / nsims) * 10000; // +100 = definite win, -100 = definite loss
}

std::pair<Move, Value> search(Board &board, int time, int side) {
    games = last_check = 0;
    start = clock();
    max_time = time;

    MCTSNode *root = new MCTSNode();
    root->parent = nullptr;
    root->val = 0;
    root->nsims = 0;
    root->move = NullMove;
    root->children.clear();

    while (games < limit) {
        if (games - last_check >= 1000) {
            if ((clock() - start) / CLOCKS_PER_MS > max_time) {
                break;
            }
            last_check = games;
            if (games%10000==0) std::cout << "info depth " << games/10000 << " time " << (clock() - start) / CLOCKS_PER_MS << " nodes " << games << " score cp " << -to_cp_eval(root->nsims, root->val) << std::endl;
        }
        select(root, board);
    }
    std::cout << "info depth " << games/10000 << " time " << (clock() - start) / CLOCKS_PER_MS << " nodes " << games << " score cp " << -to_cp_eval(root->nsims, root->val) << std::endl;

    Move best_move;
    Value best_val = -VALUE_MAX;
    int most_visited = 0;
    for (auto &child : root->children) {
        if (child->nsims > most_visited) {
            most_visited = child->nsims;
            best_move = child->move;
            best_val = child->val;
        }
    }
    clear_nodes(root);
    delete root;
    return {best_move, best_val};
}

// Phase 1: Selection
// Selects a node to explore based on UCB1
void select(MCTSNode *node, Board &board) {
    if (node->move != NullMove) board.make_move(node->move);
    if (node->children.size() == 0) {
        // If the node has no children, expand it
        expand(node, board);
        // Then, simulate the first child
        if (node->children.size() > 0) {
            MCTSNode *child = node->children[rand() % node->children.size()];
            board.make_move(child->move);
            int score = simulate(board, board.side == WHITE ? 1 : -1);
            backpropagate(child, score);
            board.unmake_move();
            games++;
        }
    } else {
        // Otherwise, select the child with the highest UCB1 value
        double best_ucb = -1e9;
        MCTSNode *best_child = nullptr;

        for (auto &child : node->children) {
            double ucb = child->ucbval();
            if (ucb > best_ucb) {
                best_ucb = ucb;
                best_child = child;
            }
        }

        if (best_child) {
            select(best_child, board);
        }
    }
    if (node->move != NullMove) board.unmake_move();
}

// Phase 2: Expansion
// Expands the node by adding a new child
void expand(MCTSNode *node, Board &board) {
    pzstd::vector<Move> moves;
    board.legal_moves(moves);

    for (int i = 0; i < moves.size(); i++) {
        Move &move = moves[i];
        MCTSNode *child = new MCTSNode();
        child->move = move;
        child->parent = node;
        node->children.push_back(child);
    }
}

// Phase 3: Simulation
// Simulates a random game from the current node
// Returns the score of the game, where 1 is a win for the POV and -1 is a loss
int simulate(Board &board, int side, int depth) {
    if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
        return -1 * side;
    }
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
        return 1 * side;
	}

    if (board.threefold() || board.halfmove >= 100 || depth >= 300) {
        return 0;
    }

    pzstd::vector<Move> moves;
    board.legal_moves(moves);

    if (moves.size() == 0)
        return 0; // This should never happen, but just in case

    Move &move = moves[rand() % moves.size()];
    board.make_move(move);
    Value score = -simulate(board, -side, depth + 1);
    board.unmake_move();
    return score;
}

// Phase 4: Backpropagation
// This is done in the other functions, as we update the node's win count and simulation count
void backpropagate(MCTSNode *node, int score) {
    while (node != nullptr) {
        node->val += score;
        node->nsims++;
        score = -score;
        node = node->parent;
    }
}