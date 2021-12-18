//
// Created by pro on 2021/12/7.
//

#include "clia.h"
#include "../z3_util.h"

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

z3::expr Z3IntLqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] < inp_list[1];
}

z3::expr Z3IntGqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] > inp_list[1];
}

z3::expr Z3IntLeqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return inp_list[0] <= inp_list[1];
}

z3::expr Z3IntGeqSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
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

z3::expr Z3ImplySemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return z3::implies(inp_list[0], inp_list[1]);
}

z3::expr Z3IteSemantics::encodeZ3Expr(const z3::expr_vector &inp_list) {
    return z3::ite(inp_list[0], inp_list[1], inp_list[2]);
}

#define RIG(x, y) ext::registerZ3Semantics(x, std::make_shared<Z3 ## y ## Semantics>())

void clia::loadZ3SemanticsForCLIA() {
    RIG("+", IntPlus); RIG("-", IntMinus); RIG("*", IntTimes); RIG("div", IntDiv);
    RIG("mod", IntMod); RIG("<", IntLq); RIG(">", IntGq); RIG("<=", IntLeq);
    RIG(">=", IntGeq); RIG("=", Eq); RIG("and", And); RIG("or", Or);
    RIG("not", Not); RIG("=>", Imply); RIG("ite", Ite);
}