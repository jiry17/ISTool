//
// Created by pro on 2022/2/16.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/solver/polygen/dnf_learner.h"
#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/solver/stun/stun.h"

FunctionContext invoker::single::invokePolyGen(Specification *spec, Verifier *v, TimeGuard *guard, const InvokeConfig& config) {
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* solver = new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);
    return solver->synthesis(guard);
}