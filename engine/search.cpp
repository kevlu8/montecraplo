#include "search.hpp"
#include <algorithm>

int games = 0, limit = 50000, last_check = 0;
clock_t start;
int max_time = 0;
double c_puct = 1.414; // PUCT exploration constant

fast_random rng(1);

void clear_nodes(MCTSNode *root) {
    for (auto &child : root->children) {
        clear_nodes(child);
        delete child;
    }
    root->children.clear();
}

int to_cp_eval(int nsims, int val) {
    if (nsims == 0) return 0;
    return ((double)val / nsims) * 10000; // +100 = definite win, -100 = definite loss
}

double score_move(Move &move, Board &board) {
    double score = 1.0; // Base score
    
    // Capture bonus
    if ((board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) & square_bits(move.dst())) {
        score += PieceValue[board.mailbox[move.dst()] & 7] / 100.0;
    }
    
    // Promotion bonus
    if (move.type() == PROMOTION) {
        score += 8.0;
    }
    
    // Piece development and central control
    Piece moving_piece = board.mailbox[move.src()];
    PieceType piece_type = PieceType(moving_piece & 7);
    
    // Central square bonus for knights and bishops
    int dst_file = move.dst() % 8;
    int dst_rank = move.dst() / 8;
    if ((piece_type == KNIGHT || piece_type == BISHOP) && 
        dst_file >= 2 && dst_file <= 5 && dst_rank >= 2 && dst_rank <= 5) {
        score += 0.8;
    }
    
    // Pawn advancement bonus
    if (piece_type == PAWN) {
        bool is_white = !(moving_piece >> 3);
        int advancement = is_white ? dst_rank : (7 - dst_rank);
        if (advancement >= 5) score += 0.5;
    }
    
    // King safety penalty for moving king in opening/middlegame
    if (piece_type == KING) {
        int piece_count = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
        if (piece_count > 20) { // Still in opening/middlegame
            score *= 0.3; // Discourage king moves when many pieces on board
        }
    }
    
    return score;
}

int ngames() {
    return games;
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
            if (games%3000==0) {
                std::cout << "info depth " << games/10000+1 << " time " << (clock() - start) / CLOCKS_PER_MS << " nodes " << games << " score cp " << -to_cp_eval(root->nsims, root->val) << " nps " << (int)(games / ((double)(clock() - start) / CLOCKS_PER_SEC));
                std::cout << " pv ";
                Move best_move;
                int most_visited = 0;
                for (auto &child : root->children) {
                    if (child->nsims > most_visited) {
                        most_visited = child->nsims;
                        best_move = child->move;
                    }
                }
                std::cout << best_move.to_string() << std::endl;
            }
        }
        select(root, board);
    }

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
    return {best_move, to_cp_eval(most_visited, best_val)};
}

// Phase 1: Selection
// Selects a node to explore based on PUCT (Predictor + UCT)
void select(MCTSNode *node, Board &board) {
    // std::cout << "selecting node " << node->move.to_string() << " games " << games;
    // std::cout << " children " << node->children.size() << " nsims " << node->nsims << " val " << node->val << std::endl;
    if (node->move != NullMove) board.make_move(node->move);
    if (node->children.size() == 0) {
        // If the node has no children, expand it
        expand(node, board);
        // Then, simulate a child
        if (node->children.size() > 0) {
            MCTSNode *child = node->children[rng.next() % node->children.size()];
            board.make_move(child->move);
            double score = -simulate(board);
            board.unmake_move();
            backpropagate(child, score);
            games++;
        } else {
            // If the node has no children, we are at a terminal node
            double score = -simulate(board);
            backpropagate(node, score);
            games++;
        }
    } else {
        // Otherwise, select the child with the highest PUCT value
        double best_puct = -1e9;
        MCTSNode *best_child = nullptr;

        for (auto &child : node->children) {
            double puct = child->puctval(c_puct);
            if (puct > best_puct) {
                best_puct = puct;
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
    if (is_game_over(board)) {
        return; // No moves to expand
    }

    pzstd::vector<Move> psuedo_moves, moves;
    board.legal_moves(psuedo_moves);

    uint8_t end = board.ended(psuedo_moves, moves);

    if (end) {
        // Stalemate or checkmate
        return;
    }

    double tot_score = 0;
    pzstd::vector<double> scores;
    for (int i = 0; i < moves.size(); i++) {
        Move &move = moves[i];
        double score = score_move(move, board);
        tot_score += score;
        scores.push_back(score);
    }

    for (int i = 0; i < moves.size(); i++) {
        Move &move = moves[i];
        MCTSNode *child = new MCTSNode();
        child->move = move;
        child->parent = node;
        // Ensure we don't divide by zero
        child->prior = tot_score > 0 ? scores[i] / tot_score : 1.0 / moves.size();
        node->children.push_back(child);
    }
}

// Phase 3: Simulation
// Simulates a game from the current node using a mix of evaluation and limited random play
// Returns the score of the game, where 1 is a win for the side to move and -1 is a loss
double simulate(Board &board, int depth) {
    if (board.threefold() || board.halfmove >= 100) {
        return 0.0; // Draw
    }

    pzstd::vector<Move> psuedo_moves, moves;
    board.legal_moves(psuedo_moves);

    uint8_t end = board.ended(psuedo_moves, moves);

    if (end == 1) {
        // Checkmate
        return -1.0;
    }

    if (end == 2) {
        // Stalemate
        return 0.0;
    }

    if (moves.size() == 0) {
        // Should never happen
        return 0.0;
    }

    int piece_count = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);

    if (piece_count <= 16) {
        double material_eval = fast_eval(board);
        if (abs(material_eval) > 0.6) {
            return board.side == WHITE ? material_eval : -material_eval;
        }
    }
    
    bool should_eval = (depth >= 15) || 
                      (piece_count <= 12) || 
                      (depth >= 8 && rng.next() % 3 == 0) ||
                      (rng.next() % 20 == 0);

    if (should_eval) {
        double eval_score = eval(board);
        if (eval_score > 0.8) return 0.9 + (eval_score - 0.8) * 0.5;
        if (eval_score < -0.8) return -0.9 + (eval_score + 0.8) * 0.5;
        if (eval_score > 0.5) return 0.5 + (eval_score - 0.5) * 1.33;
        if (eval_score < -0.5) return -0.5 + (eval_score + 0.5) * 1.33;
        return eval_score;
    }

    if (depth < 20) {
        pzstd::vector<std::pair<Move, double>> scored_moves;
        for (Move &move : moves) {
            double score = score_move(move, board);
            scored_moves.push_back({move, score});
        }

        std::sort(scored_moves.begin(), scored_moves.end(), 
                 [](const auto &a, const auto &b) { return a.second > b.second; });
        int selection_pool = std::min((int)scored_moves.size(), std::max(1, (int)scored_moves.size() / 3));
        Move &move = scored_moves[rng.next() % selection_pool].first;
        
        board.make_move(move);
        double score = -simulate(board, depth + 1);
        board.unmake_move();
        return score;
    } else {
        Move &move = moves[rng.next() % moves.size()];
        board.make_move(move);
        double score = -simulate(board, depth + 1);
        board.unmake_move();
        return score;
    }
}

// Phase 4: Backpropagation
// This is done in the other functions, as we update the node's win count and simulation count
void backpropagate(MCTSNode *node, double score) {
    while (node != nullptr) {
        node->val += score;
        node->nsims++;
        score = -score;
        node = node->parent;
    }
}

void set_puct_constant(double c) {
    c_puct = c;
}