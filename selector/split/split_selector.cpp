//
// Created by pro on 2022/1/13.
//

#include "istool/selector/split/split_selector.h"
#include "istool/solver/enum/enum_util.h"

SplitSelector::SplitSelector(Splitor* _splitor, const PSynthInfo &info, int n): splitor(_splitor) {
    std::vector<FunctionContext> info_list;
    auto* tmp_guard = new TimeGuard(0.1);
    solver::collectAccordingNum({info}, n, info_list, EnumConfig(nullptr, nullptr, tmp_guard));
    for (const auto& prog: info_list) {
        seed_list.push_back(prog.begin()->second);
    }
    delete tmp_guard;
}

bool SplitSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    addExampleCount();
    return splitor->getSplitExample(p, seed_list, counter_example);
}

SplitSelector::~SplitSelector() {
    delete splitor;
}