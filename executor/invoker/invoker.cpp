//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/selector/selector.h"
#include "glog/logging.h"

InvokeConfig::~InvokeConfig() {
    for (auto& item: item_map) delete item.second;
}

InvokeConfig::InvokeConfigItem::InvokeConfigItem(void *_data, std::function<void (void *)> _free_operator):
    data(_data), free_operator(_free_operator) {
}
InvokeConfig::InvokeConfigItem::~InvokeConfigItem() {
    free_operator(data);
}

#define RegisterSolverBuilder(name) solver = invoker::single::build ## name(spec, v, config); break

FunctionContext invoker::synthesis(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config) {
    Solver* solver = nullptr;
    switch (solver_token) {
        case SolverToken::COMPONENT_BASED_SYNTHESIS:
            RegisterSolverBuilder(CBS);
        case SolverToken::OBSERVATIONAL_EQUIVALENCE:
            RegisterSolverBuilder(OBE);
        case SolverToken::EUSOLVER:
            RegisterSolverBuilder(EuSolver);
        case SolverToken::VANILLA_VSA:
            RegisterSolverBuilder(VanillaVSA);
        case SolverToken::POLYGEN:
            RegisterSolverBuilder(PolyGen);
        case SolverToken::MAXFLASH:
            RegisterSolverBuilder(MaxFlash);
        default:
            LOG(FATAL) << "Unknown solver token";
    }
    auto res = solver->synthesis(guard);
    delete solver;
    return res;
}

std::pair<int, FunctionContext> invoker::getExampleNum(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config) {
    auto* s = dynamic_cast<Selector*>(v);
    if (s) {
        auto res = synthesis(spec, v, solver_token, guard, config);
        return {s->example_count, res};
    }
    s = new DirectSelector(v);
    auto res = synthesis(spec, s, solver_token, guard, config);
    int num = s->example_count;
    delete s;
    return {num, res};
}

namespace {
    std::unordered_map<std::string, SolverToken> token_map {
            {"cbs", SolverToken::COMPONENT_BASED_SYNTHESIS},
            {"obe", SolverToken::OBSERVATIONAL_EQUIVALENCE},
            {"eusolver", SolverToken::EUSOLVER},
            {"maxflash", SolverToken::MAXFLASH},
            {"vsa", SolverToken::VANILLA_VSA},
            {"polygen", SolverToken::POLYGEN}
    };
}

SolverToken invoker::string2TheoryToken(const std::string &name) {
    if (token_map.find(name) == token_map.end()) {
        LOG(FATAL) << "Unknown solver name " << name;
    }
    return token_map[name];
}