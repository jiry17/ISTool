//
// Created by pro on 2022/1/12.
//

#include "istool/selector/selector.h"
#include "istool/sygus/theory/basic/clia/clia.h"

void ExampleCounter::addExampleCount() {
    example_count += 1;
}

bool DirectSelector::verify(const FunctionContext &info, Example *counter_example) {
    if (counter_example) addExampleCount();
    return v->verify(info, counter_example);
}
DirectSelector::DirectSelector(Verifier *_v): v(_v) {}
DirectSelector::~DirectSelector() {delete v;}