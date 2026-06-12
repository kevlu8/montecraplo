#pragma once

#include "bitboard.hpp"

static int total_nodes = 0;

struct MCTSNode {
    double val;
    int visits;
    Move move;
    MCTSNode *parent;
    pzstd::vector<MCTSNode *> children;
    Position pos;
    bool leaf;

    MCTSNode() : val(0), visits(0), move(NullMove), parent(nullptr), leaf(true) { total_nodes++; }

    inline double ucb1(double c_puct = 1.414) {
        if (visits == 0) return INFINITY; // prioritize unexplored nodes
        // UCB1 formula: Q + C * sqrt(log(N) / n)
        // where Q is average value, C is exploration constant,
        // N is parent visits, n is node visits
        double q_value = (double)val / visits;
        double u_value = c_puct * sqrt(log(parent->visits)) / (visits);
        return q_value + u_value;
    }
};