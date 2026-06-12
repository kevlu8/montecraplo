#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "node.hpp"
#include "random.hpp"

MCTSNode *select(MCTSNode *u);
MCTSNode *expand(MCTSNode *u);
int rollout(MCTSNode *u);
void backprop(MCTSNode *u, int res);

void search(Position &pos, int time=1e9);

extern uint64_t its;
