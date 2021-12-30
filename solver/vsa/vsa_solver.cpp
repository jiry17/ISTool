//
// Created by pro on 2021/12/30.
//

#include "istool/solver/vsa/vsa_solver.h"
#include "glog/logging.h"

BasicVSASolver::~BasicVSASolver() {
    delete builder;
}
BasicVSASolver::BasicVSASolver(Specification *spec, VSABuilder *_builder, const VSAEnvPreparation& p):
    PBESolver(spec), builder(_builder), prepare(p) {
    io_space = dynamic_cast<IOExampleSpace*>(spec->example_space);
    if (!io_space) {
        LOG(FATAL) << "VSA solver supports only IO examples";
    }
}

PVSANode BasicVSASolver::buildVSA(const ExampleList &example_list, TimeGuard *guard) {
    int pos = -1; PVSANode res;
    std::string feature;
    for (int i = 0; i < example_list.size(); ++i) {
        feature += "@" + data::dataList2String(example_list[i]);
        if (cache.find(feature) != cache.end()) {
            pos = i; res = cache[feature];
        }
    }
    if (pos == -1) {
        auto io_example = io_space->getIOExample(example_list[0]);
        prepare(spec, io_example);
        res = builder->buildVSA(io_example.second, io_example.first);
        cache[data::dataList2String(example_list[0])] = res;
    }
    if (pos + 1 == example_list.size()) return res;
    ExampleList remain_examples;
    for (int i = pos + 1; i < example_list.size(); ++i) {
        remain_examples.push_back(example_list[i]);
    }
    auto remain_res = buildVSA(remain_examples, guard);
    res = builder->mergeVSA(res, remain_res);
    return cache[feature] = res;
}

FunctionContext BasicVSASolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    auto root = buildVSA(example_list, guard);
    FunctionContext res;
    res[io_space->func_name] = ext::vsa::getMinimalProgram(root);
    return res;
}