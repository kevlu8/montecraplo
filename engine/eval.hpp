#pragma once

#include "includes.hpp"
#include "bitboard.hpp"
#include "nn/accumulator.hpp"
#include "nn/network.hpp"

Value eval(Position &pos, AccumulatorManager &am);
