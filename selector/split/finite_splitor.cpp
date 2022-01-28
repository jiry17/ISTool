//
// Created by pro on 2022/1/14.
//

#include "istool/selector/split/finite_splitor.h"
#include "glog/logging.h"

FiniteSplitor::FiniteSplitor(ExampleSpace *_example_space): Splitor(_example_space), env(example_space->env) {
    io_space = dynamic_cast<FiniteIOExampleSpace*>(example_space);
    if (!io_space) {
        LOG(FATAL) << "FiniteSplitSelector supports only FiniteIOExampleSpace";
    }
    for (auto& example: io_space->example_space) {
        example_list.push_back(io_space->getIOExample(example));
    }
}

bool FiniteSplitor::getSplitExample(const PProgram &p, const ProgramList &seed_list, Example *counter_example,
        TimeGuard *guard) {
    int best_id = -1;
    int best_cost = seed_list.size() + 1;
    for (int i = 0; i < example_list.size(); ++i) {
        auto& example = example_list[i];
        if (p && env->run(p.get(), example.first) == example.second) continue;
        int cost = getCost(example.first, seed_list);
        if (cost < best_cost) {
            best_cost = cost; best_id = i;
        }
    }
    if (best_id == -1) return true;
    if (counter_example) (*counter_example) = io_space->example_space[best_id];
    return false;
}

int FiniteSplitor::getCost(const DataList &inp, const ProgramList &seed_list) {
    std::unordered_map<std::string, int> result_map;
    int cost = 0;
    for (const auto& p: seed_list) {
        auto res = env->run(p.get(), inp);
        auto feature = res.toString();
        cost = std::max(cost, result_map[feature]++);
    }
    return cost;
}