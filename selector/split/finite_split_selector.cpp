//
// Created by pro on 2022/1/14.
//

#include "istool/selector/split/finite_split_selector.h"
#include "glog/logging.h"

FiniteSplitSelector::FiniteSplitSelector(Specification *spec, int n):
    SplitSelector(spec->info_list[0], n), FiniteExampleVerifier(dynamic_cast<FiniteExampleSpace*>(spec->example_space.get())) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "FiniteSplitSelector requires the number of synthesis targets to be 1.";
    }
    auto* io_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "FiniteSplitSelector supports only FiniteIOExampleSpace";
    }
    std::vector<std::pair<int, int>> cost_list;
    for (int i = 0; i < io_space->example_space.size(); ++i) {
        auto io_example = io_space->getIOExample(io_space->example_space[i]);
        cost_list.emplace_back(getCost(io_example.first), i);
    }
    std::sort(cost_list.begin(), cost_list.end());
    for (int i = 0; i < cost_list.size(); ++i) {
        sorted_example_list.push_back(io_space->example_space[cost_list[i].second]);
    }
}

int FiniteSplitSelector::getCost(const DataList &inp) const {
    std::unordered_map<std::string, int> result_map;
    int cost = 0;
    for (const auto& p: seed_list) {
        auto res = program::run(p.get(), inp);
        auto feature = res.toString();
        cost = std::max(cost, result_map[feature]++);
    }
    return cost;
}

bool FiniteSplitSelector::verify(const FunctionContext &info, Example *counter_example) {
    if (!counter_example) return FiniteExampleVerifier::verify(info, counter_example);
    addExampleCount();
    for (const auto& example: sorted_example_list) {
        if (!example_space->satisfyExample(info, example)) {
            *counter_example = example;
            return false;
        }
    }
    return true;
}