//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/enum/enum_solver.h"

// TODO: support more kind of programs
FunctionContext invoker::single::invokeOBE(Specification *spec, Verifier *v, TimeGuard *guard) {
    ProgramChecker runnable = [](Program* p) {return true;};

    auto* obe = new OBESolver(spec, v, runnable);
    auto* solver = new CEGISSolver(obe, v);
    auto res = solver->synthesis(nullptr);
    delete solver;
    return res;
}