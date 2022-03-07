//
// Created by pro on 2022/2/21.
//

#include "istool/invoker/lifting_invoker.h"
#include "istool/solver/autolifter/autolifter.h"
#include "istool/solver/autolifter/composed_sf_solver.h"
#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/solver/polygen/dnf_learner.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/sygus/theory/basic/clia/clia_value.h"

namespace {
    int _getComponentNum(Type* type) {
        auto* pt = dynamic_cast<TProduct*>(type);
        if (!pt) return 1;
        int res = 0;
        for (auto& sub_type: pt->sub_types) {
            res += _getComponentNum(sub_type.get());
        }
        return res;
    }

    const SFSolverBuilder KDefaultSFBuilder = [](PartialLiftingTask* task) {return new ComposedSFSolver(task);};
    const SolverBuilder KDefaultSCBuilder = [](Specification* spec, Verifier* v) -> Solver* {
        auto domain_builder = solver::lia::liaSolverBuilder;
        auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
        auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());

        int total_component_num = 0;
        for (const auto& type: spec->info_list[0]->inp_type_list) {
            total_component_num += _getComponentNum(type.get());
        }
        spec->env->setConst(solver::polygen::KMaxTermNumName, BuildData(Int, std::min(4, total_component_num)));
        spec->env->setConst(solver::lia::KConstIntMaxName, BuildData(Int, 1));
        spec->env->setConst(solver::lia::KTermIntMaxName, BuildData(Int, 2));
        spec->env->setConst(solver::lia::KMaxCostName, BuildData(Int, 4));
        spec->env->setConst(solver::polygen::KMaxClauseNumName, BuildData(Int, 3));
        spec->env->setConst(solver::polygen::KIsAllowErrorName, BuildData(Bool, true));
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