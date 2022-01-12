//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SELECTOR_H
#define ISTOOL_SELECTOR_H

#include "istool/basic/env.h"

namespace selector {
    extern const std::string KSelectorTimeLimitName;
    void setSelectorTimeOut(Env* env, double ti);
    Data* getSelectorTimeOut(Env* env);
}

#endif //ISTOOL_SELECTOR_H
