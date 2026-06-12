#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "node.hpp"
#include "random.hpp"
#include "eval.hpp"

MCTSNode *select(MCTSNode *u, Position &pos, RepetitionHandler &rp);
MCTSNode *expand(MCTSNode *u, Position &pos, RepetitionHandler &rp);
double rollout(MCTSNode *u, Position &pos, RepetitionHandler &rp);
void backprop(MCTSNode *u, double res);

void search(Position &pos, RepetitionHandler &rp, int time=1e9);

extern uint64_t its;
