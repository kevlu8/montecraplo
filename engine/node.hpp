#pragma once

#include "bitboard.hpp"

constexpr double sqrt2 = sqrt(2);

struct MCTSNode {
    int nwins, nsims, npar;
    pzstd::vector<Move> children;

    MCTSNode() : nwins(0), nsims(0), npar(0) {}

    inline double ucbval() {
        return (double)nwins / nsims + sqrt2 * sqrt(log(npar) / nsims);
    }
};