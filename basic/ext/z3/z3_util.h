//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_UTIL_H
#define ISTOOL_Z3_UTIL_H

#include "basic/basic/program.h"
#include "basic/theory/theory.h"
#include "z3_semantics.h"

namespace ext {
    z3::expr encodeZ3ExprForSemantics(const PSemantics& semantics, const z3::expr_vector& inp_list, const z3::expr_vector& param_list);
    z3::expr encodeZ3ExprForProgram(const PProgram& program, const z3::expr_vector& param_list);
    z3::expr buildVar(const PType& type, const std::string& name, z3::context& ctx);
    z3::expr buildConst(const Data& data, z3::context& ctx);
    PZ3Semantics getZ3Semantics(const std::string& name);
    void registerZ3Semantics(const std::string& name, PZ3Semantics&& z3s);
    void loadZ3Theory(TheoryToken token);
    Data getValueFromModel(const z3::expr& expr, const z3::model& model, bool is_strict = false);
    extern z3::context z3_ctx;
}


#endif //ISTOOL_Z3_UTIL_H
