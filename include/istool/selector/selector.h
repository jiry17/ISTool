//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SELECTOR_H
#define ISTOOL_SELECTOR_H

#include "istool/basic/env.h"
#include "istool/basic/verifier.h"

class ExampleCounter {
public:
    int example_count = 0;
    void addExampleCount();
};

class Selector: public ExampleCounter, public Verifier {
public:
    virtual ~Selector() = default;
};

class DirectSelector: public Selector {
public:
    Verifier* v;
    DirectSelector(Verifier* _v);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~DirectSelector();
};

#endif //ISTOOL_SELECTOR_H
