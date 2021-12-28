//
// Created by pro on 2021/12/28.
//

#include "istool/sygus/theory/basic/string/string_type.h"

std::string TString::getName() {
    return "String";
}
bool TString::equal(Type *type) {
    return dynamic_cast<TString*>(type);
}

PType theory::string::getTString() {
    static PType t_string;
    if (!t_string) t_string = std::make_shared<TString>();
    return t_string;
}