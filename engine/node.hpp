#pragma once

#include "bitboard.hpp"

struct MCTSNode {
    double val;
    int nsims;
    Move move;
    MCTSNode *parent;
    pzstd::vector<MCTSNode *> children;
    double prior;

    MCTSNode() : val(0), nsims(0), move(NullMove), parent(nullptr), prior(1) {}

    inline double ucbval() {
        if (nsims == 0) return 1e9; // prioritize unexplored nodes
        // Standard UCB1 formula: Q + C * sqrt(ln(N) / n) 
        // where Q is average value, C is exploration constant, N is parent visits, n is node visits
        double exploitation = (double)val / nsims;
        double exploration = sqrt(2.0 * log(parent->nsims) / nsims);
        return exploitation + exploration;
    }
};