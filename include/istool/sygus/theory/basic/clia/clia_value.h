//
// Created by pro on 2021/12/19.
//

#ifndef ISTOOL_CLIA_VALUE_H
#define ISTOOL_CLIA_VALUE_H

#include "clia_type.h"
#include "istool/basic/value.h"

class IntValue: public Value, public TypedValue, public ComparableValue {
public:
    int w;
    IntValue(int _w);
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
    virtual bool leq(Value* value) const;
};

namespace theory {
    namespace clia {
        int getIntValue(const Data& data);
        int getBoolValue(const Data& data);
    }
}

#endif //ISTOOL_CLIA_VALUE_H
