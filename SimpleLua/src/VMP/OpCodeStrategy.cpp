#include "OpCodeStrategy.h"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

RandomizedStrategy::RandomizedStrategy() {
    std::vector<int> values;
    // OpCode is contiguous from 0 to OP_RETURN
    for (int i = 0; i <= OP_RETURN; ++i) {
        values.push_back(i);
    }

    // Shuffle
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(values.begin(), values.end(), std::default_random_engine(seed));

    // Map
    for (int i = 0; i <= OP_RETURN; ++i) {
        opMap[static_cast<OpCode>(i)] = values[i];
    }
}

int RandomizedStrategy::get(OpCode op) const {
    auto it = opMap.find(op);
    if (it != opMap.end()) {
        return it->second;
    }
    return static_cast<int>(op);
}
