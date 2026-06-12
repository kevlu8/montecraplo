#include "includes.hpp"

#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

bool dfrc_uci = false;

int main(int argc, char *argv[]) {
	if (argc == 2 && std::string(argv[1]) == "bench") {
		Position pos = Position();
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		search(pos, 1000);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout << 1 << " nodes " << (int)(its / ((double)(elapsed) / 1000.0)) << " nps" << std::endl;
		return 0;
	}
	std::cout << "MonteCraplo " << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	Position pos = Position();
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name MonteCraplo " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			pos = Position();
		} else if (command.substr(0, 8) == "position") {
			// either `position startpos` or `position fen ...`
			if (command.find("startpos") != std::string::npos) {
				pos = Position();
			} else if (command.find("fen") != std::string::npos) {
				std::string fen = command.substr(command.find("fen") + 4);
				if (fen.find("moves") != std::string::npos) {
					fen = fen.substr(0, fen.find("moves"));
				}
				pos = Position(fen);
			}
			if (command.find("moves") != std::string::npos) {
				std::string moves = command.substr(command.find("moves") + 6);
				std::stringstream ss(moves);
				std::string move;
				while (ss >> move) {
					pos.make_move(Move::from_string(move, &pos));
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
			int timeleft = pos.side ? btime : wtime;
			int inc = pos.side ? binc : winc;
			if (inf)
				search(pos, 1e9);
			else
				search(pos, timemgmt(timeleft, inc));
		}
	}
}
