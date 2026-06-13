#include "includes.hpp"

#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

#include "nn/accumulator.hpp"

bool dfrc_uci = false;
Network nn;

struct PosInfo {
	Position pos;
	int result;
	alignas(64) std::array<float, 4096 + 72> policy;
};

void datagen(const std::string& filename) {
	std::ifstream openings(filename);
	std::ofstream output("data.txt");
	std::string line;
	Position *pos;
	RepetitionHandler *rp = new RepetitionHandler();
	AccumulatorManager *am;
	int total_pos = 0;
	while (std::getline(openings, line)) {
		// Start the game from this fen
		pos = new Position(line);
		rp->clear();
		rp->push_hash(pos->zobrist_without_ep());
		am = new AccumulatorManager(*pos);

		int res = 0, plies = 0;
		pzstd::vector<PosInfo, 1024> positions;

		while (true) {
			// Check for game end
			if (plies >= 512 || pos->halfmove >= 100 || pos->insufficient_material() || rp->threefold(0, pos->zobrist_without_ep())) break;

			// Verify that there exists a legal move
			pzstd::vector<Move> moves;
			pos->pseudolegal_moves(moves);
			bool legal_exists = false;
			for (const auto &m : moves) {
				if (pos->is_legal(m)) {
					legal_exists = true;
					break;
				}
			}

			if (!legal_exists) {
				if (pos->checkers[pos->side]) {
					// stm has lost
					res = pos->side == WHITE ? -1 : 1;
				}
				break;
			}

			PosInfo info;
			info.pos = *pos;

			// find move
			Move m = search(*pos, *rp, *am, 1e9, 100, &info.policy);
			pos->make_move(m);
			rp->push_hash(pos->zobrist_without_ep());
			am->make_move(*pos, m);
			positions.push_back(info);
			plies++;
			total_pos++;
		}

		// res is based on white
		for (auto &info : positions) {
			info.result = info.pos.side == WHITE ? res : -res;
		}

		for (const auto &info : positions) {
			bool w_input[768] = {}, b_input[768] = {};
			for (int i = 0; i < 64; i++) {
				w_input[calculate_index(Square(i), PieceType(info.pos.mailbox[i] & 7), info.pos.mailbox[i] >> 3, 0)] = true;
				b_input[calculate_index(Square(i), PieceType(info.pos.mailbox[i] & 7), info.pos.mailbox[i] >> 3, 1)] = true;
			}
			if (info.pos.side == WHITE) {
				// put white's input first
				output.write(reinterpret_cast<char*>(w_input), 768 * sizeof(bool));
				output.write(reinterpret_cast<char*>(b_input), 768 * sizeof(bool));
			} else {
				output.write(reinterpret_cast<char*>(b_input), 768 * sizeof(bool));
				output.write(reinterpret_cast<char*>(w_input), 768 * sizeof(bool));
			}
			output.write(reinterpret_cast<const char*>(&info.result), sizeof(int));
			output.write(reinterpret_cast<const char*>(info.policy.data()), PHEAD_SIZE * sizeof(float));
			output.write("\n", 1);
		}

		std::cout << total_pos << " positions" << std::endl;

		delete pos;
		delete am;
	}
	openings.close();
	output.close();
	delete rp;
}

int main(int argc, char *argv[]) {
	nn.load();
	Position *pos = new Position();
	RepetitionHandler *rp = new RepetitionHandler();
	AccumulatorManager *am = new AccumulatorManager(*pos);
	if (argc == 2 && std::string(argv[1]) == "bench") {
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		search(*pos, *rp, *am, 1000);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout << 1 << " nodes " << (int)(its / ((double)(elapsed) / 1000.0)) << " nps" << std::endl;
		delete pos;
		delete rp;
		delete am;
		return 0;
	}
	if (argc == 3 && std::string(argv[1]) == "datagen") {
		datagen(argv[2]);
	}
	std::cout << "MonteCraplo " << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name MonteCraplo " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			delete pos;
			delete am;
			pos = new Position();
			rp->clear();
			am = new AccumulatorManager(*pos);
		} else if (command.substr(0, 8) == "position") {
			// either `position startpos` or `position fen ...`
			if (command.find("startpos") != std::string::npos) {
				delete pos; pos = new Position();
				rp->clear(); rp->push_hash(pos->zobrist_without_ep());
				delete am;
				am = new AccumulatorManager(*pos);
			} else if (command.find("fen") != std::string::npos) {
				std::string fen = command.substr(command.find("fen") + 4);
				if (fen.find("moves") != std::string::npos) {
					fen = fen.substr(0, fen.find("moves"));
				}
				delete pos;
				pos = new Position(fen);
				rp->clear(); rp->push_hash(pos->zobrist_without_ep());
				delete am;
				am = new AccumulatorManager(*pos);
			}
			if (command.find("moves") != std::string::npos) {
				std::string moves = command.substr(command.find("moves") + 6);
				std::stringstream ss(moves);
				std::string move;
				while (ss >> move) {
					Move m = Move::from_string(move, &pos);
					am->make_move(*pos, m);
					pos->make_move(m);
					rp->push_hash(pos->zobrist_without_ep());
				}
			}
		} else if (command == "quit") {
			break;
		} else if (command == "stop") {
			// stop the search thread
			// if (searchthread.joinable()) {
			// 	searchthread.join();
			// }
		} else if (command.substr(0, 2) == "go") {
			// `go wtime ... btime ... winc ... binc ...`
			// only care about wtime and btime
			std::stringstream ss(command);
			std::string token;
			int wtime = 0, btime = 0, winc = 0, binc = 0;
			int depth = -1;
			int nodes = -1;
			bool inf = false;
			ss >> token;
			while (ss >> token) {
				if (token == "wtime") {
					ss >> wtime;
				} else if (token == "btime") {
					ss >> btime;
				} else if (token == "winc") {
					ss >> winc;
				} else if (token == "binc") {
					ss >> binc;
				} else if (token == "infinite") {
					inf = true;
				}
			}
			int timeleft = pos->side ? btime : wtime;
			int inc = pos->side ? binc : winc;
			if (inf)
				search(*pos, *rp, *am, 1e9);
			else
				search(*pos, *rp, *am, timemgmt(timeleft, inc));
		}
	}
}
