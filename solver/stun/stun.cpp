//
// Created by pro on 2022/1/3.
//

#include "istool/solver/stun/stun.h"
#include "glog/logging.h"

STUNSolver::STUNSolver(Specification* spec, TermSolver *_term_solver, Unifier *_unifier):
    term_solver(_term_solver), unifier(_unifier), PBESolver(spec) {
    if (spec->info_list.size() != 1) {
        LOG(FATAL) << "STUNSolver can only synthesize a single program";
    }
    func_name = spec->info_list[0]->name;
}

FunctionContext STUNSolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    auto term_list = term_solver->synthesisTerms(example_list, guard);
    FunctionContext res;
    res[func_name] = unifier->unify(term_list, example_list, guard);
    return res;
}