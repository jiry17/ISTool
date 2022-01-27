//
// Created by pro on 2022/1/27.
//

#ifndef ISTOOL_SAMPLESY_H
#define ISTOOL_SAMPLESY_H

#include "istool/selector/selector.h"
#include "istool/selector/split/splitor.h"

class SampleSyCore {
public:
    virtual void addExample(const Example& example) = 0;
    virtual bool isAllEquivalent() = 0;
};

class SampleSy: public Selector {
public:
    Splitor* splitor;
    SampleSy(Splitor* _splitor);
    virtual void addExample() = 0;
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    virtual ~SampleSy() = default;
};

#endif //ISTOOL_SAMPLESY_H
