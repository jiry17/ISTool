//
// Created by pro on 2022/1/13.
//

#ifndef ISTOOL_SPLIT_SELECTOR_H
#define ISTOOL_SPLIT_SELECTOR_H

#include "istool/selector/selector.h"

class SplitSelector: public Selector {
public:
    ProgramList seed_list;
    SplitSelector(const PSynthInfo& info, int n);
    virtual ~SplitSelector() = default;
};


#endif //ISTOOL_SPLIT_SELECTOR_H
