//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/selector/selector.h"
#include "glog/logging.h"

FunctionContext invoker::synthesis(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard) {
    switch (solver_token) {
        case SolverToken::COMPONENT_BASED_SYNTHESIS:
            return invoker::single::invokeCBS(spec, v, guard);
        case SolverToken::OBSERVATIONAL_EQUIVALENCE:
            return invoker::single::invokeOBE(spec, v, guard);
        case SolverToken::EUSOLVER:
            return invoker::single::invokeEuSolver(spec, v, guard);
        default:
            LOG(FATAL) << "Unknown solver token";
    }
}

std::pair<int, FunctionContext> invoker::getExampleNum(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard) {
    auto* s = dynamic_cast<Selector*>(v);
    if (s) {
        auto res = synthesis(spec, v, solver_token, guard);
        return {s->example_count, res};
    }
    s = new DirectSelector(v);
    auto res = synthesis(spec, v, solver_token, guard);
    int num = s->example_count;
    delete s;
    return {num, res};
}

namespace {
    std::unordered_map<std::string, SolverToken> token_map {
            {"cbs", SolverToken::COMPONENT_BASED_SYNTHESIS},
            {"obe", SolverToken::OBSERVATIONAL_EQUIVALENCE},
            {"eusolver", SolverToken::EUSOLVER}
    };
}

SolverToken invoker::string2TheoryToken(const std::string &name) {
    if (token_map.find(name) == token_map.end()) {
        LOG(FATAL) << "Unknown solver name " << name;
    }
    return token_map[name];
}