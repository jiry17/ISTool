//
// Created by pro on 2022/1/4.
//

#include "istool/solver/polygen/lia_solver.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "glog/logging.h"

namespace {
    const int KDefaultValue = 2;

    int _getDefaultConstMax(Grammar* g) {
        int c_max = KDefaultValue;
        for (auto* symbol: g->symbol_list) {
            for (auto* rule: symbol->rule_list) {
                auto* cs = dynamic_cast<ConstSemantics*>(rule->semantics.get());
                if (!cs) continue;
                auto* iv = dynamic_cast<IntValue*>(cs->w.get());
                if (iv) c_max = std::max(c_max, std::abs(iv->w));
            }
        }
        return c_max;
    }
}

LIASolver::LIASolver(Specification *_spec): PBESolver(_spec), ext(ext::z3::getExtension(_spec->env)) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "LIA Solver can only synthesize a single program";
    }
    io_example_space = dynamic_cast<IOExampleSpace*>(spec->example_space);
    if (!io_example_space) {
        LOG(FATAL) << "LIA solver supports only IOExampleSpace";
    }
    auto* c_max_data = spec->env->getConstRef(solver::lia::KConstIntMaxName);
    if (c_max_data->isNull()) {
        spec->env->setConst(solver::lia::KConstIntMaxName,
                BuildData(Int, _getDefaultConstMax(spec->info_list[0]->grammar)));
    }
    const_int_max = theory::clia::getIntValue(*c_max_data);
    auto* t_max_data = spec->env->getConstRef(solver::lia::KTermIntMaxName);
    if (t_max_data->isNull()) {
        spec->env->setConst(solver::lia::KTermIntMaxName, BuildData(Int, KDefaultValue));
    }
    term_int_max = theory::clia::getIntValue(*t_max_data);
}
FunctionContext LIASolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    z3::optimize o(ext->ctx);
    for (auto& example: example_list) {
        auto io_example = io_example_space->getIOExample(example);
        
    }
}