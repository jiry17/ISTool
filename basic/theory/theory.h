//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_THEORY_H
#define ISTOOL_THEORY_H

#include "basic/basic/specification.h"

enum class TheoryToken {
    CLIA, BV
};

class TheorySpecification: public Specification {
public:
    TheoryToken theory;
    TheorySpecification(const std::vector<PSynthInfo>& info_list, TypeSystem* _type_system, Verifier* _v, TheoryToken _token);
    void loadSemantics(const std::function<void(TheoryToken)>& loader);
    ~TheorySpecification() = default;
};

namespace semantics {
    VTAssigner loadTheory(TheoryToken token);
}

#endif //ISTOOL_THEORY_H
