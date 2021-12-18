//
// Created by pro on 2021/12/3.
//

#include "type.h"

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