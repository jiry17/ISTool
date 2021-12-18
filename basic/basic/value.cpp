//
// Created by pro on 2021/11/30.
//

#include "value.h"

TypedValue::TypedValue(PType &&_type): type(_type) {}

SyGuSValue::SyGuSValue(PType &&_type): TypedValue(std::move(_type)) {}

NullValue::NullValue(): TypedValue(std::make_shared<TBot>()) {}

bool NullValue::equal(Value *value) const {
    auto* nv = dynamic_cast<NullValue*>(value);
    return nv;
}

std::string NullValue::toString() const {
    return "null";
}

IntValue::IntValue(int _w): SyGuSValue(std::make_shared<TInt>()), w(_w) {
}
bool IntValue::equal(Value *value) const {
    auto* iv = dynamic_cast<IntValue*>(value);
    return iv && iv->w == w;
}
std::string IntValue::toString() const {
    return std::to_string(w);
}
z3::expr IntValue::getZ3Expr(z3::context &ctx) const {
    return ctx.int_val(w);
}

BoolValue::BoolValue(bool _w): SyGuSValue(std::make_shared<TBool>()), w(_w) {
}
bool BoolValue::equal(Value *value) const {
    auto* bv = dynamic_cast<BoolValue*>(value);
    return bv && bv->w == w;
}
std::string BoolValue::toString() const {
    if (w) return "true"; else return "false";
}
z3::expr BoolValue::getZ3Expr(z3::context &ctx) const {
    return ctx.bool_val(w);
}