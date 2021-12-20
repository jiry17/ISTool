//
// Created by pro on 2021/12/18.
//

#ifndef ISTOOL_Z3_EXTENSION_H
#define ISTOOL_Z3_EXTENSION_H

#include "istool/basic/env.h"
#include "istool/basic/program.h"
#include "z3_type.h"
#include "z3_semantics.h"

#include <list>

class Z3Extension: public Extension {
    std::list<Z3Type*> util_list;
    std::unordered_map<std::string, Z3Semantics*> semantics_pool;
    Z3Type* getZ3Type(Type* type) const;
    Z3Semantics* getZ3Semantics(Semantics* semantics) const;
public:
    void registerZ3Type(Z3Type* util);
    void registerZ3Semantics(const std::string& name, Z3Semantics* semantics);
    z3::context ctx;

    z3::expr buildVar(Type* type, const std::string& name);
    z3::expr buildConst(const Data& data);
    z3::expr encodeZ3ExprForSemantics(Semantics* semantics, const z3::expr_vector& inp_list, const z3::expr_vector& param_list);
    z3::expr encodeZ3ExprForProgram(Program* program, const z3::expr_vector& param_list);
    Data getValueFromModel(const z3::model& model, const z3::expr& expr, Type* type, bool is_strict = false);
    virtual ~Z3Extension();
};

namespace ext {
    namespace z3 {
        Z3Extension* getZ3Extension(Env* env);
    }
}


#endif //ISTOOL_Z3_EXTENSION_H
