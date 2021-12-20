//
// Created by pro on 2021/11/30.
//

#ifndef ISTOOL_VLAUE_H
#define ISTOOL_VLAUE_H

#include <string>
#include <memory>
#include "type.h"

class Value {
public:
    virtual ~Value() = default;
    virtual std::string toString() const = 0;
    virtual bool equal(Value* value) const = 0;
};

typedef std::shared_ptr<Value> PValue;

class ComparableValue {
public:
    virtual bool leq(Value* value) const = 0;
};

class TypedValue {
public:
    PType type;
    TypedValue(PType&& _type);
};

class NullValue: public Value, public TypedValue {
public:
    NullValue();
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
};

class BoolValue: public Value, public TypedValue {
public:
    bool w;
    BoolValue(bool _w);
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
};

#endif //ISTOOL_VLAUE_H
