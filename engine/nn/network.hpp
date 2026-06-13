#pragma once

#include "../includes.hpp"

#define INPUT_SIZE 768
#define L1_SIZE 16
#define L2_SIZE 16
#define PHEAD_SIZE 4168
#define VHEAD_SIZE 1
#define SCALE 400

struct Accumulator {
	alignas(32) float val[L1_SIZE] = {};
};

struct Network {
	float accumulator_weights[INPUT_SIZE][L1_SIZE];
	float accumulator_biases[L1_SIZE];

	float l1_weights[L2_SIZE][L1_SIZE * 2];
	float l1_biases[L2_SIZE];

	float policy_weights[PHEAD_SIZE][L2_SIZE];
	float policy_biases[PHEAD_SIZE];

	float value_weights[VHEAD_SIZE][L2_SIZE];
	float value_biases[VHEAD_SIZE];

	void load();
};

int calculate_index(Square sq, PieceType pt, bool side, bool perspective);

std::array<float, PHEAD_SIZE> nn_policy(const Network &net, const Accumulator &stm, const Accumulator &ntm);
float nn_value(const Network &net, const Accumulator &stm, const Accumulator &ntm);

extern Network nn;
