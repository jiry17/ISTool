//
// Created by pro on 2021/12/9.
//

#include "istool/solver/solver.h"
#include "glog/logging.h"
#include <mutex>

Solver::Solver(Specification *_spec): spec(_spec) {}
PBESolver::PBESolver(Specification *_spec): spec(_spec) {}

CEGISSolver::CEGISSolver(PBESolver *_pbe_solver, Verifier *_v):
    pbe_solver(_pbe_solver), v(_v), Solver(_pbe_solver->spec) {
}
CEGISSolver::~CEGISSolver() {
    delete pbe_solver; delete v;
}
FunctionContext CEGISSolver::synthesis(TimeGuard* guard) {
    std::vector<Example> example_list;
    while (1) {
        auto res = pbe_solver->synthesis(example_list, guard);
        Example counter_example;
        if (v->verify(res, &counter_example)) {
            return res;
        }
        example_list.push_back(counter_example);
    }
}