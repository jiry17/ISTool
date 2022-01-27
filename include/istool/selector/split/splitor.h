//
// Created by pro on 2022/1/27.
//

#ifndef ISTOOL_SPLITOR_H
#define ISTOOL_SPLITOR_H

#include "istool/basic/env.h"
#include "istool/basic/example_space.h"
#include "istool/basic/program.h"
#include "istool/basic/time_guard.h"

class Splitor {
protected:
    virtual bool getSplitExample(const PProgram& p, const ProgramList& seed_list, Example* counter_example,
            TimeGuard* guard) = 0;
public:
    ExampleSpace* example_space;
    Data* KSplitTimeOut;
    Splitor(ExampleSpace* _example_space);
    bool getSplitExample(const PProgram& p, const ProgramList& seed_list, Example* counter_example);
    ~Splitor() = default;
};

namespace selector {
    void setSplitorTimeOut(Env* env, double ti);
    Data* getSplitorTimeOut(Env* env);
}


#endif //ISTOOL_SPLITOR_H
