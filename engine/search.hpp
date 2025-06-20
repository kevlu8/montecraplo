#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "node.hpp"
#include "eval.hpp"
#include "random.hpp"
#include "util.hpp"

int ngames();

void select(MCTSNode *node, Board &board);
void expand(MCTSNode *node, Board &board);
double simulate(Board &board, int depth=0);
void backpropagate(MCTSNode *node, double score);

void set_puct_constant(double c);

std::pair<Move, Value> search(Board &board, int time=1e9, int side=1);

