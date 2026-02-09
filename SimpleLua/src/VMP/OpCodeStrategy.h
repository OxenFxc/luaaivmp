#ifndef OPCODESTRATEGY_H
#define OPCODESTRATEGY_H

#include "../OpCodes.h"
#include <vector>

class OpCodeStrategy {
public:
    virtual ~OpCodeStrategy() = default;
    virtual int get(OpCode op) const = 0;
};

class DefaultStrategy : public OpCodeStrategy {
public:
    int get(OpCode op) const override {
        return static_cast<int>(op);
    }
};

class RandomizedStrategy : public OpCodeStrategy {
private:
    std::vector<int> opMap;

public:
    RandomizedStrategy();
    int get(OpCode op) const override;
};

#endif
