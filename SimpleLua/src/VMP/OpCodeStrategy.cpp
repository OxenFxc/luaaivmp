#include "OpCodeStrategy.h"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

RandomizedStrategy::RandomizedStrategy() {
    // OpCode is contiguous from 0 to OP_RETURN
    opMap.resize(OP_RETURN + 1);
    for (int i = 0; i <= OP_RETURN; ++i) {
        opMap[i] = i;
    }

    // Shuffle
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(opMap.begin(), opMap.end(), std::default_random_engine(seed));
}

int RandomizedStrategy::get(OpCode op) const {
    if (static_cast<size_t>(op) < opMap.size()) {
        return opMap[static_cast<size_t>(op)];
    }
    return static_cast<int>(op);
}
