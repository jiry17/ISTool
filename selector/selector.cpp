//
// Created by pro on 2022/1/12.
//

#include "istool/selector/selector.h"
#include "istool/sygus/theory/basic/clia/clia.h"

extern const std::string selector::KSelectorTimeLimitName = "Selector@TimeOut";

void Selector::addExampleCount() {
    example_count += 1;
}

bool DirectSelector::verify(const FunctionContext &info, Example *counter_example) {
    if (counter_example) addExampleCount();
    return v->verify(info, counter_example);
}
DirectSelector::DirectSelector(Verifier *_v): v(_v) {}
DirectSelector::~DirectSelector() {delete v;}

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