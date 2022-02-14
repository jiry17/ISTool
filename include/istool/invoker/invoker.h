//
// Created by pro on 2022/2/15.
//

#ifndef ISTOOL_INVOKER_H
#define ISTOOL_INVOKER_H

#include "istool/basic/specification.h"
#include "istool/basic/time_guard.h"
#include "istool/basic/verifier.h"

enum class SolverToken {
    OBSERVATIONAL_EQUIVALENCE,
    COMPONENT_BASED_SYNTHESIS,
    EUSOLVER
};

namespace invoker {
    namespace single {
        FunctionContext invokeCBS(Specification* spec, Verifier* v, TimeGuard* guard);
        FunctionContext invokeOBE(Specification* spec, Verifier* v, TimeGuard* guard);
        FunctionContext invokeEuSolver(Specification* spec, Verifier* v, TimeGuard* guard);
    }

    FunctionContext synthesis(Specification* spec, Verifier* v, SolverToken solver_token, TimeGuard* guard);
    std::pair<int, FunctionContext> getExampleNum(Specification* spec, Verifier* v, SolverToken solver_token, TimeGuard* guard);
    SolverToken string2TheoryToken(const std::string& name);
}

#endif //ISTOOL_INVOKER_H
