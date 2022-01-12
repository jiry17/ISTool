//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SPLIT_SELECTOR_H
#define ISTOOL_SPLIT_SELECTOR_H

#include "istool/ext/z3/z3_verifier.h"

class Z3SplitSelector: public Z3Verifier {
public:
    Z3IOExampleSpace* io_space;
    ProgramList seed_list;
    Data* time_out;
    PSynthInfo info;
    int example_num = 0;
    Z3SplitSelector(Specification* spec, int n);
    virtual bool verify(const FunctionContext& res, Example* counter_example);
};

#endif //ISTOOL_SPLIT_SELECTOR_H
