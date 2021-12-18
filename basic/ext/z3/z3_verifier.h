//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_VERIFIER_H
#define ISTOOL_Z3_VERIFIER_H

#include "../../basic/verifier.h"

struct Z3CallInfo {
    std::string name;
    z3::expr_vector inp_symbols;
    z3::expr oup_symbol;
    Z3CallInfo(const std::string& _name, const z3::expr_vector& _inp_symbols, const z3::expr& _oup_symbol);
};

class Z3IOVerifier: public Verifier {
public:
    z3::expr_vector cons_list;
    Z3CallInfo info;
    Z3IOVerifier(const z3::expr_vector& _cons_list, const Z3CallInfo& _info);
    virtual PExample verify(const PProgram& program) const;
    virtual ~Z3IOVerifier() = default;
};


#endif //ISTOOL_Z3_VERIFIER_H
