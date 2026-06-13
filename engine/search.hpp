#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "node.hpp"
#include "random.hpp"
#include "eval.hpp"
#include "util.hpp"
#include "nn/accumulator.hpp"

extern Network nn;

MCTSNode *select(MCTSNode *u, Position &pos, RepetitionHandler &rp, AccumulatorManager &am);
void expand(MCTSNode *u, Position &pos, RepetitionHandler &rp);
double rollout(MCTSNode *u, Position &pos, AccumulatorManager &am);
void backprop(MCTSNode *u, double res);

Move search(Position &pos, RepetitionHandler &rp, AccumulatorManager &am, int time=1e9, uint64_t visits=1e18, void *opt_visdistr=nullptr);

extern uint64_t its;
