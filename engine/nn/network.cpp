#include "network.hpp"

#include "incbin.h"

extern "C" {
	INCBIN(network_weights, NN_PATH);
}

void Network::load() {
	char *ptr = (char *)gnetwork_weightsData;

	memcpy(accumulator_weights, ptr, sizeof(accumulator_weights));
	ptr += sizeof(accumulator_weights);

	memcpy(accumulator_biases, ptr, sizeof(accumulator_biases));
	ptr += sizeof(accumulator_biases);

	memcpy(l1_weights, ptr, sizeof(l1_weights));
	ptr += sizeof(l1_weights);

	memcpy(l1_biases, ptr, sizeof(l1_biases));
	ptr += sizeof(l1_biases);
	
	memcpy(policy_weights, ptr, sizeof(policy_weights));
	ptr += sizeof(policy_weights);

	memcpy(policy_biases, ptr, sizeof(policy_biases));
	ptr += sizeof(policy_biases);

	memcpy(value_weights, ptr, sizeof(value_weights));
	ptr += sizeof(value_weights);

	memcpy(value_biases, ptr, sizeof(value_biases));
	ptr += sizeof(value_biases);
}

int calculate_index(Square sq, PieceType pt, bool side, bool perspective) {
	if (perspective) {
		side = !side;
		sq = (Square)(sq ^ 56);
	}
	return side * 64 * 6 + pt * 64 + sq;
}

std::array<float, PHEAD_SIZE> nn_policy(const Network &net, const Accumulator &stm, const Accumulator &ntm) {
	std::array<float, PHEAD_SIZE> policy;

	float l2[L2_SIZE];
	for (int i = 0; i < L2_SIZE; i++) {
		l2[i] = net.l1_biases[i];
		for (int j = 0; j < L1_SIZE; j++) {
			l2[i] += fmaxf(0, stm.val[j]) * net.l1_weights[i][j];
			l2[i] += fmaxf(0, ntm.val[j]) * net.l1_weights[i][j + L1_SIZE];
		}
		l2[i] = fmaxf(0, l2[i]); // ReLU
	}

	for (int i = 0; i < PHEAD_SIZE; i++) {
		policy[i] = net.policy_biases[i];
		for (int j = 0; j < L2_SIZE; j++) {
			policy[i] += l2[j] * net.policy_weights[i][j];
		}
	}

	return policy;
}

float nn_value(const Network &net, const Accumulator &stm, const Accumulator &ntm) {
	float l2[L2_SIZE];
	for (int i = 0; i < L2_SIZE; i++) {
		l2[i] = net.l1_biases[i];
		for (int j = 0; j < L1_SIZE; j++) {
			l2[i] += fmaxf(0, stm.val[j]) * net.l1_weights[i][j];
			l2[i] += fmaxf(0, ntm.val[j]) * net.l1_weights[i][j + L1_SIZE];
		}
		l2[i] = fmaxf(0, l2[i]); // ReLU
	}

	float value = net.value_biases[0];
	for (int i = 0; i < L2_SIZE; i++) {
		value += l2[i] * net.value_weights[0][i];
	}

	return std::tanh(value);
}
