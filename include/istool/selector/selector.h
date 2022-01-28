//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SELECTOR_H
#define ISTOOL_SELECTOR_H

#include "istool/basic/env.h"
#include "istool/basic/verifier.h"
#include "istool/solver/solver.h"

class ExampleCounter {
public:
    int example_count = 0;
    void addExampleCount();
};

class Selector: public ExampleCounter, public Verifier {
public:
    virtual ~Selector() = default;
};

class CompleteSelector: public ExampleCounter, public Solver {
public:
    IOExampleSpace* io_space;
    CompleteSelector(Specification* spec);
    virtual PProgram getNextAction(Example* example) = 0;
    virtual void addExample(const IOExample& example) = 0;
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
    virtual ~CompleteSelector() = default;
};

class DirectSelector: public Selector {
public:
    Verifier* v;
    DirectSelector(Verifier* _v);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~DirectSelector();
};

#endif //ISTOOL_SELECTOR_H
