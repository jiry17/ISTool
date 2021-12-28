//
// Created by pro on 2021/12/28.
//

#include "istool/sygus/theory/basic/string/string_value.h"
#include "istool/sygus/theory/basic/string/string_type.h"
#include "glog/logging.h"

StringValue::StringValue(const std::string &_s): s(_s), TypedValue(theory::string::getTString()) {
}
std::string StringValue::toString() const {
    return "\"" + s + "\"";
}
bool StringValue::equal(Value *value) const {
    auto* sv = dynamic_cast<StringValue*>(value);
    if (!sv) {
        LOG(FATAL) << "Expect StringValue, but get " << value->toString();
    }
    return s == sv->s;
}

std::string theory::string::getStringValue(const Data &d) {
    auto* sv = dynamic_cast<StringValue*>(d.get());
    if (!sv) {
        LOG(FATAL) << "Expect StringValue, but get " << d.toString();
    }
    return sv->s;
}