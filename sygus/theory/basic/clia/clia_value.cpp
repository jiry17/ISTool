//
// Created by pro on 2021/12/19.
//

#include "istool/sygus/theory/basic/clia/clia_value.h"
#include "glog/logging.h"


IntValue::IntValue(int _w): w(_w), TypedValue(theory::clia::getTInt()) {
}
bool IntValue::equal(Value *value) const {
    auto* iv = dynamic_cast<IntValue*>(value);
    if (!iv) {
        LOG(FATAL) << "Expect IntValue, but get" << value->toString();
    }
    return iv->w == w;
}
std::string IntValue::toString() const {
    return std::to_string(w);
}
bool IntValue::leq(Value *value) const {
    auto* iv = dynamic_cast<IntValue*>(value);
    if (!iv) {
        LOG(FATAL) << "Expect IntValue, but get " << value->toString();
    }
    return w < iv->w;
}

int theory::clia::getIntValue(const Data &data) {
    auto* iv = dynamic_cast<IntValue*>(data.get());
    if (!iv) {
        LOG(FATAL) << "Expected IntValue, but get " << data.toString();
    }
    return iv->w;
}
int theory::clia::getBoolValue(const Data& data) {
    auto* bv = dynamic_cast<BoolValue*>(data.get());
    if (!bv) {
        LOG(FATAL) << "Expected BoolValue, but get " << data.toString();
    }
    return bv->w;
}
