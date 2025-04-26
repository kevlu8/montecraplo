#pragma once

#include "bitboard.hpp"

struct MCTSNode {
    int val, nsims;
    Move move;
    MCTSNode *parent;
    pzstd::vector<MCTSNode *> children;
    double prior;

    MCTSNode() : val(0), nsims(0), move(NullMove), parent(nullptr), prior(1) {}

    inline double ucbval() {
        if (nsims == 0) return 1e9; // prioritize unexplored nodes
        return (double)val / nsims + prior * sqrt(2 * log(parent->nsims) / nsims);
    }
};