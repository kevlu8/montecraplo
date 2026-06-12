#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "node.hpp"
#include "random.hpp"

MCTSNode *select(MCTSNode *u, Position &pos);
MCTSNode *expand(MCTSNode *u, Position &pos);
int rollout(MCTSNode *u, Position &pos, RepetitionHandler &rp);
void backprop(MCTSNode *u, int res);

void search(Position &pos, RepetitionHandler &rp, int time=1e9);

extern uint64_t its;
