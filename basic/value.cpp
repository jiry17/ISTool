//
// Created by pro on 2021/11/30.
//

#include "value.h"

TypedValue::TypedValue(PType &&_type): type(_type) {}

NullValue::NullValue(): TypedValue(std::make_shared<TBot>()) {}

bool NullValue::equal(Value *value) const {
    auto* nv = dynamic_cast<NullValue*>(value);
    return nv;
}

std::string NullValue::toString() const {
    return "null";
}