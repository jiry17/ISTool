//
// Created by pro on 2022/5/3.
//

#ifndef ISTOOL_RANDOM_SEMANTICS_SELECTOR_H
#define ISTOOL_RANDOM_SEMANTICS_SELECTOR_H

#include "istool/selector/selector.h"
#include "istool/selector/random/random_semantics_scorer.h"
#include "istool/ext/z3/z3_verifier.h"

class BasicRandomSemanticsSelector {
public:
    Env* env;
    Grammar* g;
    RandomSemanticsScorer* scorer;
    ExampleList history_inp_list;
    BasicRandomSemanticsSelector(Env* env, Grammar* _g, RandomSemanticsScorer* _scorer);
    int getBestExampleId(const PProgram& program, const ExampleList& candidate_list);
    void addHistoryExample(const Example& inp);
    ~BasicRandomSemanticsSelector();
};

class FiniteRandomSemanticsSelector: public Verifier, public BasicRandomSemanticsSelector {
public:
    FiniteIOExampleSpace* io_space;
    IOExampleList io_example_list;
    FiniteRandomSemanticsSelector(Specification* spec, RandomSemanticsScorer* scorer);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~FiniteRandomSemanticsSelector() = default;
};

class Z3RandomSemanticsSelector: public Z3Verifier, public BasicRandomSemanticsSelector {
public:
    Z3IOExampleSpace* z3_io_space;
    int KExampleNum;
    Z3RandomSemanticsSelector(Specification* spec, RandomSemanticsScorer* scorer, int _KExampleNum);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~Z3RandomSemanticsSelector() = default;
};

#endif //ISTOOL_RANDOM_SEMANTICS_SELECTOR_H
