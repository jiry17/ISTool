//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_CLIA_H
#define ISTOOL_CLIA_H

#include "basic/theory/clia_semantics.h"
#include "basic/ext/z3/z3_semantics.h"

#define DeclareZ3Semantics(name) \
class Z3 ## name ## Semantics: public Z3Semantics { \
public: \
    virtual z3::expr encodeZ3Expr(const z3::expr_vector& inp_list); \
};

DeclareZ3Semantics(IntPlus)
DeclareZ3Semantics(IntMinus)
DeclareZ3Semantics(IntTimes)
DeclareZ3Semantics(IntDiv)
DeclareZ3Semantics(IntMod)
DeclareZ3Semantics(IntGq)
DeclareZ3Semantics(IntLq)
DeclareZ3Semantics(IntGeq)
DeclareZ3Semantics(IntLeq)
DeclareZ3Semantics(Eq)
DeclareZ3Semantics(Not)
DeclareZ3Semantics(And)
DeclareZ3Semantics(Or)
DeclareZ3Semantics(Imply)
DeclareZ3Semantics(Ite)

namespace clia {
    void loadZ3SemanticsForCLIA();
}

#endif //ISTOOL_CLIA_H
