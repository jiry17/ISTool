//
// Created by pro on 2022/2/21.
//

#include "istool/invoker/lifting_invoker.h"
#include "istool/solver/autolifter/autolifter.h"
#include "istool/solver/autolifter/composed_sf_solver.h"
#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/solver/polygen/dnf_learner.h"

namespace {
    const SFSolverBuilder KDefaultSFBuilder = [](PartialLiftingTask* task) {return new ComposedSFSolver(task);};
    const SolverBuilder KDefaultSCBuilder = [](Specification* spec, Verifier* v) -> Solver* {
        auto domain_builder = solver::lia::liaSolverBuilder;
        auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
        auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
        return new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);
    };
}

AutoLifter* invoker::single::buildAutoLifter(LiftingTask *task, const InvokeConfig &config) {
    auto sf_builder = config.access("SfBuilder", KDefaultSFBuilder);
    auto sc_builder = config.access("ScBuilder", KDefaultSCBuilder);
    auto* solver = new AutoLifter(task, sf_builder, sc_builder);
    return solver;
}

LiftingRes invoker::single::invokeAutoLifter(LiftingTask *task, TimeGuard *guard, const InvokeConfig &config) {
    auto* solver = invoker::single::buildAutoLifter(task, config);
    auto res = solver->synthesis(guard);
    delete solver;
    return res;
}