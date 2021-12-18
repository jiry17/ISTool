//
// Created by pro on 2021/12/3.
//

#include "type.h"

#define CompleteDefBasicType(name) \
std::string T ## name::getName() { \
    return #name; \
} \
bool T ## name::equal(Type* type) { \
    return dynamic_cast<T ## name*>(type); \
}

CompleteDefBasicType(Bool)
CompleteDefBasicType(Int)
CompleteDefBasicType(Bot)

z3::expr TBool::buildVar(const std::string &name, z3::context &ctx) {
    return ctx.bool_const(name.c_str());
}

z3::expr TInt::buildVar(const std::string& name, z3::context& ctx) {
    return ctx.int_const(name.c_str());
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

PType type::getTInt() {
    static PType t_int;
    if (!t_int) t_int = std::make_shared<TInt>();
    return t_int;
}

PType type::getTBool() {
    static PType t_bool;
    if (!t_bool) t_bool = std::make_shared<TBool>();
    return t_bool;
}

PType type::getTVarA() {
    static PType t_var;
    if (!t_var) t_var = std::make_shared<TVar>("a");
    return t_var;
}