//
// Created by pro on 2021/12/19.
//

#include "istool/sygus/theory/basic/clia/clia_type.h"

bool TInt::equal(Type *type) {
    return dynamic_cast<TInt*>(type);
}
std::string TInt::getName() {
    return "Int";
}

PType theory::clia::getTInt() {
    static PType int_type = nullptr;
    if (!int_type) {
        int_type = std::make_shared<TInt>();
    }
    return int_type;
}

PType theory::clia::getTBool() {
    return type::getTBool();
}