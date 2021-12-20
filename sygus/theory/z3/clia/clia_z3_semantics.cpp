//
// Created by pro on 2021/12/20.
//

#include "istool/sygus/theory/z3/clia/clia_z3_semantics.h"
#include "istool/ext/z3/z3_extension.h"

z3::expr Z3IntPlusSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] + inp_list[1];
}
z3::expr Z3IntMinusSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] - inp_list[1];
}
z3::expr Z3IntTimesSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] * inp_list[1];
}
z3::expr Z3IntDivSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] / inp_list[1];
}
z3::expr Z3IntModSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] % inp_list[1];
}
z3::expr Z3LqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] < inp_list[1];
}
z3::expr Z3LeqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] <= inp_list[1];
}
z3::expr Z3GqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] > inp_list[1];
}
z3::expr Z3GeqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] >= inp_list[1];
}
z3::expr Z3EqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] == inp_list[1];
}
z3::expr Z3NotSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return !inp_list[0];
}
z3::expr Z3AndSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] && inp_list[1];
}
z3::expr Z3OrSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] || inp_list[1];
}
z3::expr Z3IteSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return z3::ite(inp_list[0], inp_list[1], inp_list[2]);
}

#define LoadZ3Semantics(name, sem) z3_env->registerZ3Semantics(name, new Z3 ## sem ## Semantics())

void theory::clia::loadZ3Semantics(Env *env) {
    auto* z3_env = ext::z3::getZ3Extension(env);
    LoadZ3Semantics("+", IntPlus); LoadZ3Semantics("-", IntMinus);
    LoadZ3Semantics("*", IntTimes); LoadZ3Semantics("div", IntDiv);
    LoadZ3Semantics("mod", IntMod); LoadZ3Semantics("<", Lq);
    LoadZ3Semantics("<=", Leq); LoadZ3Semantics(">", Gq);
    LoadZ3Semantics(">=", Geq); LoadZ3Semantics("=", Eq);
    LoadZ3Semantics("=", Eq); LoadZ3Semantics("!", Not);
    LoadZ3Semantics("&&", And); LoadZ3Semantics("||", Or);
    LoadZ3Semantics("ite", Ite);
}