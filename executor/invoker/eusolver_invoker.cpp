//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/stun/stun.h"
#include "istool/solver/stun/eusolver.h"

FunctionContext invoker::single::invokeEuSolver(Specification *spec, Verifier *v, TimeGuard *guard) {
    auto split_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* eusolver = new EuSolver(spec, split_info.first, split_info.second);
    auto* s = new CEGISSolver(eusolver, v);
    auto res = s->synthesis(guard);
    delete s;
    return res;
}