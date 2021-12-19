//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_VERIFIER_H
#define ISTOOL_Z3_VERIFIER_H

#include "basic/verifier.h"
#include "basic/env.h"
#include "z3_extension.h"

struct Z3Info {
    z3::expr expr;
    PType type;
    Z3Info(const z3::expr& _expr, PType& _type);
};
typedef std::vector<Z3Info> Z3InfoList;

struct Z3CallInfo {
    std::string name;
    Z3InfoList inp_infos;
    Z3Info oup_info;
    z3::expr_vector getInpList() const;
    Z3CallInfo(const std::string& _name, const Z3InfoList& _inp_symbols, const Z3Info& _oup_symbol);
};

class Z3IOVerifier: public Verifier {
public:
    z3::expr_vector cons_list;
    Z3CallInfo info;
    Z3Extension* z3_env;
    Z3IOVerifier(const z3::expr_vector& _cons_list, const Z3CallInfo& _info, Env* _env);
    virtual PExample verify(const PProgram& program) const;
    virtual ~Z3IOVerifier() = default;
};


#endif //ISTOOL_Z3_VERIFIER_H
