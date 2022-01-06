//
// Created by pro on 2022/1/7.
//

#include "istool/solver/polygen/dnf_learner.h"
#include "glog/logging.h"
#include <set>

namespace {
    const std::set<std::string> ignored_set = {
            "||", "&&", "!", "and", "not", "or"
    };

    PSynthInfo _buildPredInfo(const PSynthInfo& info) {
        auto* g = grammar::copyGrammar(info->grammar);
        int now = 0;
        for (auto* rule: g->start->rule_list) {
            auto name = rule->semantics->getName();
            if (ignored_set.find(name) != ignored_set.end()) {
                delete rule;
            } else g->start->rule_list[now++] = rule;
        }
        g->start->rule_list.resize(now);
        g->removeUseless();
        return std::make_shared<SynthInfo>(info->name, info->inp_type_list, info->oup_type, g);
    }
}

DNFLearner::DNFLearner(Specification *spec): PBESolver(spec) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "DNFLearner can only synthesize a single program";
    }
    auto dnf_info = spec->info_list[0];
    auto bool_type = type::getTBool();
    if (!bool_type->equal(dnf_info->oup_type.get())) {
        LOG(FATAL) << "DNFLearner: the output type must be Boolean";
    }
    pred_info = _buildPredInfo(spec);
}