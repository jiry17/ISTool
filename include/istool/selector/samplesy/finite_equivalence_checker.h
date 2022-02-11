//
// Created by pro on 2022/1/28.
//

#ifndef ISTOOL_FINITE_EQUIVALENCE_CHECKER_H
#define ISTOOL_FINITE_EQUIVALENCE_CHECKER_H

#include "istool/selector/selector.h"
#include "different_program_generator.h"
#include <stack>

class FiniteEquivalenceChecker: public EquivalenceChecker{
public:
    DifferentProgramGenerator* g;
    IOExampleList example_list;
    FiniteEquivalenceChecker(DifferentProgramGenerator* _g, FiniteIOExampleSpace* io_space);
    virtual void addExample(const IOExample& example);
    virtual ProgramList getTwoDifferentPrograms();
    virtual ~FiniteEquivalenceChecker() = default;
};

#endif //ISTOOL_FINITE_EQUIVALENCE_CHECKER_H
