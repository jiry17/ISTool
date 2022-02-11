//
// Created by pro on 2022/1/13.
//

#include "istool/selector/split/split_selector.h"
#include "istool/solver/enum/enum_util.h"

SplitSelector::SplitSelector(Splitor* _splitor, const PSynthInfo &info, int n, Env* env): splitor(_splitor) {
    std::vector<FunctionContext> info_list;
    auto* tmp_guard = new TimeGuard(0.1);
    solver::collectAccordingNum({info}, n, info_list, EnumConfig(nullptr, nullptr, tmp_guard));
    for (const auto& prog: info_list) {
        seed_list.push_back(prog.begin()->second);
    }
    auto invoke = std::make_shared<InvokeSemantics>(info->name, env);
    ProgramList sub_list;
    for (int i = 0; i < info->inp_type_list.size(); ++i) sub_list.push_back(program::buildParam(i));
    auto left = std::make_shared<Program>(invoke, sub_list);
    auto right = program::buildParam(info->inp_type_list.size());
    sub_list = {left, right};
    cons_program = std::make_shared<Program>(env->getSemantics("!="), sub_list);
    delete tmp_guard;
}

bool SplitSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    addExampleCount();

    return splitor->getSplitExample(cons_program.get(), info, seed_list, counter_example);
}

SplitSelector::~SplitSelector() {
    delete splitor;
}

CompleteSplitSelector::CompleteSplitSelector(Specification *_spec, Splitor *_splitor, EquivalenceChecker *_checker, int n):
    CompleteSelector(_spec, _checker), splitor(_splitor) {
    std::vector<FunctionContext> info_list;
    auto* tmp_guard = new TimeGuard(0.1);
    solver::collectAccordingNum(spec->info_list, n, info_list, EnumConfig(nullptr, nullptr, tmp_guard));
    for (const auto& prog: info_list) {
        seed_list.push_back(prog.begin()->second);
    }

    auto info = spec->info_list[0];

    ProgramList cmp_list;
    for (const auto& name: {"f", "g"}) {
        auto invoke = std::make_shared<InvokeSemantics>(name, spec->env.get());
        ProgramList sub_list;
        for (int i = 0; i < info->inp_type_list.size(); ++i) sub_list.push_back(program::buildParam(i));
        auto left = std::make_shared<Program>(invoke, sub_list);
        auto right = program::buildParam(info->inp_type_list.size());
        sub_list = {left, right};
        cmp_list.push_back(std::make_shared<Program>(spec->env->getSemantics("!="), sub_list));
    }
    cons_program = std::make_shared<Program>(spec->env->getSemantics("||"), cmp_list);

    delete tmp_guard;
}

Example CompleteSplitSelector::getNextExample(const PProgram &x, const PProgram &y) {
    Example example;
    FunctionContext info;
    info["f"] = x; info["g"] = y;
    splitor->getSplitExample(cons_program.get(), info, seed_list, &example);
    return example;
}

void CompleteSplitSelector::addExample(const IOExample &example) {
    checker->addExample(example);
}

CompleteSplitSelector::~CompleteSplitSelector() {
    delete checker; delete splitor;
}