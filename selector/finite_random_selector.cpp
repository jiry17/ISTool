//
// Created by pro on 2022/2/6.
//

#include "istool/selector/finite_random_selector.h"
#include "glog/logging.h"

FiniteRandomSelector::FiniteRandomSelector(Specification *spec, EquivalenceChecker *_checker): CompleteSelector(spec, _checker) {
    finite_io_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!finite_io_space) {
        LOG(FATAL) << "FiniteRandomSelector supports only FiniteIOSpace";
    }
    env = spec->env.get();
}

void FiniteRandomSelector::addExample(const IOExample &example) {}
Example FiniteRandomSelector::getNextExample(const PProgram &x, const PProgram &y) {
    std::vector<int> id_list(finite_io_space->example_space.size());
    for (int i = 0; i < id_list.size(); ++i) id_list[i] = i;
    auto info_x = semantics::buildSingleContext(finite_io_space->func_name, x);
    auto info_y = semantics::buildSingleContext(finite_io_space->func_name, y);
    std::shuffle(id_list.begin(), id_list.end(), env->random_engine);
    for (auto& id: id_list) {
        auto& example = finite_io_space->example_space[id];
        if (finite_io_space->satisfyExample(info_x, example) && finite_io_space->satisfyExample(info_y, example)) continue;
        return example;
    }
    assert(0);
}