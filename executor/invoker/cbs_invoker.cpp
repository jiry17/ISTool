//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/component/component_based_solver.h"
#include "istool/solver/component/linear_encoder.h"

FunctionContext invoker::single::invokeCBS(Specification *spec, Verifier *v, TimeGuard* guard, const InvokeConfig& config) {
    std::unordered_map<std::string, Z3GrammarEncoder*> encoder_list;
    for (auto& info: spec->info_list) {
        encoder_list[info->name] = new LinearEncoder(info->grammar, spec->env.get());
    }
    auto* s = new CEGISSolver(new ComponentBasedSynthesizer(spec, encoder_list), v);
    auto res = s->synthesis(guard);
    delete s;
    return res;
}