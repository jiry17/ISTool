//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/enum/enum_solver.h"

namespace {
    ProgramChecker _default_runnable = [](Program* p) {return true;};
}

FunctionContext invoker::single::invokeOBE(Specification *spec, Verifier *v, TimeGuard *guard, const InvokeConfig& config) {
    auto runnable = config.access("runnable", _default_runnable);

    auto* obe = new OBESolver(spec, v, runnable);
    auto* solver = new CEGISSolver(obe, v);
    auto res = solver->synthesis(nullptr);
    delete solver;
    return res;
}