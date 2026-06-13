#pragma once

#include "bitboard.hpp"

static int total_nodes = 0;

struct MCTSNode {
	double val;
	int visits;
	Move move;
	MCTSNode *parent;
	MCTSNode *first_child, *next_sibling;
	bool terminal;

	MCTSNode() : val(0), visits(0), move(NullMove), parent(nullptr), first_child(nullptr), next_sibling(nullptr), terminal(false) { total_nodes++; }

	inline double puct(double P) {
        if (visits == 0) return INFINITY; // prioritize unexplored nodes
        // PUCT formula: Q + C * P * sqrt(N) / (1 + n)
        // where Q is average value, C is exploration constant,
        // N is parent visits, n is node visits
        double q_value = (double)val / visits;
        double u_value = 1.414 * P * sqrt(parent->visits) / (1 + visits);
        return q_value + u_value;
	}
};