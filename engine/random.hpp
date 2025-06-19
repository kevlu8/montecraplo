#pragma once

#include <cstdint>

struct fast_random {
	uint64_t state;

	fast_random(uint64_t seed) : state(seed) {}

	uint64_t next() {
		state ^= state >> 12;
		state ^= state << 25;
		state ^= state >> 27;
		return state * 0x2545F4914F6C21C6ULL; // Multiply by a large prime
	}

	int next_int(int min, int max) {
		return (int)(next() % (max - min + 1)) + min;
	}
};