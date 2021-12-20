//
// Created by pro on 2021/12/20.
//

#include "istool/sygus/theory/z3/theory_z3_semantics.h"
#include "istool/sygus/theory/z3/clia/clia_z3.h"
#include "glog/logging.h"

void theory::loadZ3Semantics(Env *env, TheoryToken token) {
    switch (token) {
        case TheoryToken::CLIA: {
            theory::loadZ3CLIA(env);
            return;
        }
            // case TheoryToken::STRING:
            // case TheoryToken::BV:
        default:
            LOG(FATAL) << "Unsupported theory";
    }
}