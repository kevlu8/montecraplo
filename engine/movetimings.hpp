#pragma once

#include "includes.hpp"

inline uint64_t timemgmt(int64_t remtime, int64_t inc = 0) {
	// Return time in ms that we can spend on this move
	return std::max(1ll, (long long)(remtime / 25 + inc * 3 / 5));
}
