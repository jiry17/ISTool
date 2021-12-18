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

class ComparableValue: virtual public Value {
public:
    virtual bool leq(ComparableValue* value) const = 0;
};

class TypedValue {
public:
    PType type;
    TypedValue(PType&& _type);
};

class Z3Value {
public:
    virtual z3::expr getZ3Expr(z3::context& ctx) const = 0;
};

class SyGuSValue: public Value, public TypedValue, public Z3Value {
public:
    SyGuSValue(PType&& _type);
};

class NullValue: public Value, public TypedValue {
public:
    NullValue();
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
};

class IntValue: public SyGuSValue {
public:
    int w;
    IntValue(int _w);
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
    virtual z3::expr getZ3Expr(z3::context& ctx) const;
};

class BoolValue: public SyGuSValue {
public:
    bool w;
    BoolValue(bool _w);
    virtual std::string toString() const;
    virtual bool equal(Value* value) const;
    virtual z3::expr getZ3Expr(z3::context& ctx) const;
};

#endif //ISTOOL_VLAUE_H
