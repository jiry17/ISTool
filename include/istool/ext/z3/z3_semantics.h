//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_SEMANTICS_H
#define ISTOOL_Z3_SEMANTICS_H

#include "z3++.h"
#include "istool/basic/semantics.h"

class Z3Semantics {
public:
    virtual z3::expr encodeZ3Expr(const z3::expr_vector& inp_list) = 0;
};

typedef std::shared_ptr<Z3Semantics> PZ3Semantics;

#define DefineZ3Semantics(name) \
class Z3 ## name ## Semantics: public Z3Semantics { \
public: \
    virtual z3::expr encodeZ3Expr(const z3::expr_vector& inp_list); \
};


#endif //ISTOOL_Z3_SEMANTICS_H
