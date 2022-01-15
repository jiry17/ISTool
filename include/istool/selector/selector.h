//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SELECTOR_H
#define ISTOOL_SELECTOR_H

#include "istool/basic/env.h"
#include "istool/basic/verifier.h"

class Selector {
public:
    int example_count = 0;
    void addExampleCount();
};

class DirectSelector: public Selector, public Verifier {
public:
    Verifier* v;
    DirectSelector(Verifier* _v);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~DirectSelector();
};

namespace selector {
    extern const std::string KSelectorTimeLimitName;
    void setSelectorTimeOut(Env* env, double ti);
    Data* getSelectorTimeOut(Env* env);
}

#endif //ISTOOL_SELECTOR_H
