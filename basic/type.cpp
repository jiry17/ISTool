//
// Created by pro on 2021/12/3.
//

#include "istool/basic/type.h"

std::string TBot::getName() {
    return "Bot";
}
bool TBot::equal(Type *type) {
    return dynamic_cast<TBot*>(type);
}

TVar::TVar(const std::string &_name): name(_name) {
}
bool TVar::equal(Type *type) {
    auto* tv = dynamic_cast<TVar*>(type);
    if (tv) return tv->name == name;
    return false;
}
std::string TVar::getName() {
    return name;
}

bool type::equal(const PType &t1, const PType &t2) {
    return t1->equal(t2.get());
}

bool TBool::equal(Type* type) {
    return dynamic_cast<TBool*>(type);
}
std::string TBool::getName() {
    return "Bool";
}

PType type::getTBool() {
    static PType bool_type = nullptr;
    if (!bool_type) {
        bool_type = std::make_shared<TBool>();
    }
    return bool_type;
}

std::string type::typeList2String(const TypeList &type_list) {
    std::string res = "[";
    for (int i = 0; i < type_list.size(); ++i) {
        if (i) res += ",";
        res += type_list[i]->getName();
    }
    return res + "]";
}