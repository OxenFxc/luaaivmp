#include "OpCodeStrategy.h"
#include <vector>
#include <algorithm>
#include <random>

RandomizedStrategy::RandomizedStrategy() {
    // OpCode is contiguous from 0 to OP_RETURN
    opMap.resize(OP_RETURN + 1);
    for (int i = 0; i <= OP_RETURN; ++i) {
        opMap[i] = i;
    }

    // Shuffle
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(values.begin(), values.end(), g);

    // Map
    for (int i = 0; i <= OP_RETURN; ++i) {
        opMap[static_cast<OpCode>(i)] = values[i];
    }
}

int RandomizedStrategy::get(OpCode op) const {
    if (static_cast<size_t>(op) < opMap.size()) {
        return opMap[static_cast<size_t>(op)];
    }
    return static_cast<int>(op);
}
