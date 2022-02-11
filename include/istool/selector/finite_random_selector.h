//
// Created by pro on 2022/2/6.
//

#ifndef ISTOOL_FINITE_RANDOM_SELECTOR_H
#define ISTOOL_FINITE_RANDOM_SELECTOR_H

#include "selector.h"

class FiniteRandomSelector: public CompleteSelector {
public:
    FiniteIOExampleSpace* finite_io_space;
    Env* env;
    FiniteRandomSelector(Specification* spec, EquivalenceChecker* _checker);
    virtual Example getNextExample(const PProgram& x, const PProgram& y);
    virtual void addExample(const IOExample& example);
    ~FiniteRandomSelector() = default;
};

#endif //ISTOOL_FINITE_RANDOM_SELECTOR_H
