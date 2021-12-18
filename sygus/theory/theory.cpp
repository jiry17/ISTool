//
// Created by pro on 2021/12/7.
//

#include "theory.h"
#include "clia_semantics.h"

VTAssigner semantics::loadTheory(TheoryToken token) {
    if (token == TheoryToken::CLIA) {
        return clia::loadTheoryCLIA();
    }
}

TheorySpecification::TheorySpecification(const std::vector<PSynthInfo>& info_list, TypeSystem *_type_system, Verifier *_v, TheoryToken _theory):
    Specification(info_list, _type_system, _v), theory(_theory) {
}

void TheorySpecification::loadSemantics(const std::function<void (TheoryToken)> &loader) {
    loader(theory);
}