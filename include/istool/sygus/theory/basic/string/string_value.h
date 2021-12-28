//
// Created by pro on 2021/12/28.
//

#ifndef ISTOOL_STRING_VALUE_H
#define ISTOOL_STRING_VALUE_H

#include "istool/basic/value.h"
#include "istool/basic/data.h"

class StringValue: public Value, public TypedValue {
public:
    std::string s;
    StringValue(const std::string& _s);
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
};

namespace theory {
    namespace string {
        std::string getStringValue(const Data& d);
    }
}

#endif //ISTOOL_STRING_VALUE_H
