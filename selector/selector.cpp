//
// Created by pro on 2022/1/12.
//

#include "istool/selector/selector.h"
#include "istool/sygus/theory/basic/clia/clia.h"

extern const std::string selector::KSelectorTimeLimitName = "Selector@TimeOut";

namespace {
    const int KDefaultTimeLimit = 1000;
}

Data* selector::getSelectorTimeOut(Env *env) {
    auto* data = env->getConstRef(selector::KSelectorTimeLimitName);
    if (data->isNull()) {
        env->setConst(selector::KSelectorTimeLimitName, BuildData(Int, KDefaultTimeLimit));
    }
    return data;
}

void selector::setSelectorTimeOut(Env *env, double ti) {
    int ti_ms = int(ti * 1000);
    env->setConst(selector::KSelectorTimeLimitName, BuildData(Int, ti_ms));
}