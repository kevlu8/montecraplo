#pragma once

#include "includes.hpp"
#include "move.hpp"

inline int move_to_policy(const Move& m) {
	if (m.type() == PROMOTION && m.promotion() + KNIGHT != QUEEN) {
		// Underpromotion
		int src_file = m.src() % 8;
		int dst_file = m.dst() % 8;
		return 4096 + src_file * 8 * 3 + (dst_file - src_file + 1) * 3 + m.promotion() - 1;
	}
	return m.src() * 64 + m.dst();
}