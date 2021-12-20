//
// Created by pro on 2021/12/20.
//

#include "istool/sygus/theory/basic/theory_semantics.h"
#include "istool/sygus/theory/basic/clia/clia_semantics.h"
#include "glog/logging.h"

void theory::loadBasicSemantics(Env *env, TheoryToken token) {
    switch (token) {
        case TheoryToken::CLIA: {
            theory::loadCLIASemantics(env);
            return;
        }
        // case TheoryToken::STRING:
        // case TheoryToken::BV:
        default:
            LOG(FATAL) << "Unsupported theory";
    }
}