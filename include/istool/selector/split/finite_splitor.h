//
// Created by pro on 2022/1/14.
//

#ifndef ISTOOL_FINITE_SPLITOR_H
#define ISTOOL_FINITE_SPLITOR_H

#include "splitor.h"
#include "istool/basic/verifier.h"

class FiniteSplitor: public Splitor {
protected:
    virtual bool getSplitExample(Program* cons_program, const FunctionContext& info,
                                 const ProgramList& seed_list, Example* counter_example, TimeGuard* guard);
    int getCost(const DataList& inp, const ProgramList& seed_list);
public:
    IOExampleList example_list;
    FiniteIOExampleSpace* io_space;
    Env* env;
    FiniteSplitor(ExampleSpace* _example_space);
    virtual ~FiniteSplitor() = default;
};

#endif //ISTOOL_FINITE_SPLITOR_H
