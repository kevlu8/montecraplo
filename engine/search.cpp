#include "search.hpp"

int games = 0, limit = 500000, last_check = 0;
clock_t start;
int max_time = 0;

std::pair<Move, Value> search(Board &board, int time, int side) {
    start = clock();
    max_time = time;

    Move best_move = NullMove;
    double best_score = -100;

    pzstd::vector<Move> moves;
    board.legal_moves(moves);

    // TODO: order moves

    for (int i = 0; i < moves.size(); i++) {
        games = 0;
        Move &move = moves[i];
        board.make_move(move);
        double score = -play(board, -side);
        board.unmake_move();

        std::cout << "info curmove " << move.to_string() << " eval " << score << std::endl;
        
        if (score > best_score) {
            best_move = move;
            best_score = score;
        }
    }

    return {best_move, best_score};
}

double play(Board &board, int side) {
    if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
        games++;
		return 100 * side;
    }
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
        games++;
		return -100 * side;
	}

    if (board.threefold() || board.halfmove >= 100) {
        games++;
        return 0;
    }

    pzstd::vector<Move> moves;
    board.legal_moves(moves);

    Value cumscore = 0;
    int games_played = 0;

    for (int i = 0; i < 2; i++) {
        Move &move = moves[rand() % moves.size()];
        board.make_move(move);
        cumscore += -play(board, -side);
        board.unmake_move();
        games_played++;
        if (games - last_check > 100) {
            if (games > limit) break;
            if ((clock() - start) / CLOCKS_PER_MS > max_time) {
                break;
            }
        }
    }

    cumscore /= games_played;

    return cumscore;
}