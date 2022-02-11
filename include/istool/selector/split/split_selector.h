//
// Created by pro on 2022/1/13.
//

#ifndef ISTOOL_SPLIT_SELECTOR_H
#define ISTOOL_SPLIT_SELECTOR_H

#include "istool/selector/selector.h"
#include "istool/selector/samplesy/samplesy.h"
#include "splitor.h"

class SplitSelector: public Selector {
public:
    Splitor* splitor;
    ProgramList seed_list;
    PProgram cons_program;
    SplitSelector(Splitor* _selector, const PSynthInfo& info, int n, Env* env);
    bool verify(const FunctionContext& info, Example* counter_example);
    virtual ~SplitSelector();
};

class CompleteSplitSelector: public CompleteSelector {
public:
    Splitor* splitor;
    ProgramList seed_list;
    PProgram cons_program;
    Data* KSampleTimeOut, *KSampleNum;
    CompleteSplitSelector(Specification* _spec, Splitor* _splitor, EquivalenceChecker* _checker, int n);
    virtual Example getNextExample(const PProgram& x, const PProgram& y);
    virtual void addExample(const IOExample& example);
    virtual ~CompleteSplitSelector();
};


#endif //ISTOOL_SPLIT_SELECTOR_H
