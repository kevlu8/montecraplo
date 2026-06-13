#include "eval.hpp"

extern Network nn;

Value eval(Position &pos, AccumulatorManager &am) {
	am.apply_lazy(pos);

	Value score = 0;

	if (pos.side == WHITE) {
		score = nn_value(nn, am.current().w_acc, am.current().b_acc);
	} else {
		score = nn_value(nn, am.current().b_acc, am.current().w_acc);
	}

	return score;
}
