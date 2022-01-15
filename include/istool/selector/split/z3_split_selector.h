//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_Z3_SPLIT_SELECTOR_H
#define ISTOOL_Z3_SPLIT_SELECTOR_H

#include "istool/ext/z3/z3_verifier.h"
#include "split_selector.h"

class Z3SplitSelector: public Z3Verifier, public SplitSelector {
public:
    Z3IOExampleSpace* io_space;
    Data* time_out;
    PSynthInfo info;
    Z3SplitSelector(Specification* spec, int n);
    virtual bool verify(const FunctionContext& res, Example* counter_example);
};

#endif //ISTOOL_Z3_SPLIT_SELECTOR_H
