//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_SEMANTICS_H
#define ISTOOL_Z3_SEMANTICS_H

#include "z3++.h"
#include "basic/basic/semantics.h"

class Z3Semantics {
public:
    virtual z3::expr encodeZ3Expr(const z3::expr_vector& inp_list) = 0;
};

typedef std::shared_ptr<Z3Semantics> PZ3Semantics;


#endif //ISTOOL_Z3_SEMANTICS_H
