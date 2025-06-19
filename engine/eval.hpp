#pragma once

#include "includes.hpp"
#include "bitboard.hpp"
#include "nn/network.hpp"

double eval(Board &board);

double fast_eval(Board &board);