//
// Created by pro on 2021/12/9.
//

#include "istool/solver/solver.h"
#include "glog/logging.h"
#include <mutex>

Solver::Solver(Specification *_spec): spec(_spec) {}
VerifiedSolver::VerifiedSolver(Specification *spec, Verifier *_v): Solver(spec), v(_v) {}
//TODO: delete v;
VerifiedSolver::~VerifiedSolver() {/*delete v;*/}
PBESolver::PBESolver(Specification *_spec): spec(_spec) {}

CEGISSolver::CEGISSolver(PBESolver *_pbe_solver, Verifier *_v):
    pbe_solver(_pbe_solver), VerifiedSolver(_pbe_solver->spec, _v) {
}
CEGISSolver::~CEGISSolver() {
    delete pbe_solver;
}
FunctionContext CEGISSolver::synthesis(TimeGuard* guard) {
    std::vector<Example> example_list;
    while (1) {
        auto res = pbe_solver->synthesis(example_list, guard);
        LOG(INFO) << "Find " << res.toString();
        for (auto example: example_list) LOG(INFO) << "res " << data::dataList2String(example) << " " << spec->env->run(res.begin()->second.get(), example).toString();
        Example counter_example;
        if (v->verify(res, &counter_example)) {
            return res;
        }
        LOG(INFO) << "Counter example " << data::dataList2String(counter_example);
        example_list.push_back(counter_example);
    }
}