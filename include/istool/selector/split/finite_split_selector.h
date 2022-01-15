//
// Created by pro on 2022/1/14.
//

#ifndef ISTOOL_FINITE_SPLIT_SELECTOR_H
#define ISTOOL_FINITE_SPLIT_SELECTOR_H

#include "split_selector.h"

class FiniteSplitSelector: public FiniteExampleVerifier, public SplitSelector {
    int getCost(const DataList& inp) const;
    ExampleList sorted_example_list;
public:
    FiniteSplitSelector(Specification* spec, int n);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    virtual ~FiniteSplitSelector() = default;
};

#endif //ISTOOL_FINITE_SPLIT_SELECTOR_H
