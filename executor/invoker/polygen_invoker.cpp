//
// Created by pro on 2022/2/16.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/solver/polygen/dnf_learner.h"
#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/solver/stun/stun.h"

Solver * invoker::single::buildPolyGen(Specification *spec, Verifier *v, const InvokeConfig &config) {
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());

    auto is_staged = config.access("is_staged", false);
    if (is_staged) {
        return new StagedCEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder);
    } else {
        return new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);
    }
}

Solver* invoker::single::buildCondSolver(Specification *spec, Verifier *v, const InvokeConfig &config) {
    auto* dnf_learner = new DNFLearner(spec);
    auto* solver = new CEGISSolver(dnf_learner, v);
    return solver;
}

Solver* invoker::single::buildLIASolver(Specification* spec, Verifier* v, const InvokeConfig& config) {
    auto* pbe_solver = solver::lia::liaSolverBuilder(spec);
    auto* cegis_solver = new CEGISSolver(pbe_solver, v);
    return cegis_solver;
}