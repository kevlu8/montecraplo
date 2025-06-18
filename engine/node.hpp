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

    inline double puctval(double c_puct = 1.414) {
        if (nsims == 0) return 1e9; // prioritize unexplored nodes
        // PUCT formula: Q + C * P * sqrt(N) / (1 + n)
        // where Q is average value, C is exploration constant, P is prior probability,
        // N is parent visits, n is node visits
        double q_value = (double)val / nsims;
        double u_value = c_puct * prior * sqrt(parent->nsims) / (1.0 + nsims);
        return q_value + u_value;
    }
};