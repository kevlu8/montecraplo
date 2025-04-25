#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"

// Plays out a game from the current board position
// Returns the evaluation on a scale from -100 to 100
double play(Board &board, int side);

std::pair<Move, Value> search(Board &board, int time=1e9, int side=1);

