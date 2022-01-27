//
// Created by pro on 2022/1/13.
//

#ifndef ISTOOL_SPLIT_SELECTOR_H
#define ISTOOL_SPLIT_SELECTOR_H

#include "istool/selector/selector.h"
#include "splitor.h"

class SplitSelector: public Selector {
public:
    Splitor* splitor;
    ProgramList seed_list;
    SplitSelector(Splitor* _selector, const PSynthInfo& info, int n);
    bool verify(const FunctionContext& info, Example* counter_example);
    virtual ~SplitSelector();
};


#endif //ISTOOL_SPLIT_SELECTOR_H
