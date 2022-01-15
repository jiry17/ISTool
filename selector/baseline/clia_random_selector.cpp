//
// Created by pro on 2022/1/12.
//

#include "istool/selector/baseline/clia_random_selector.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "glog/logging.h"

const std::string selector::KCLIARandomRangeName = "CLIA-Random@Range";

namespace {
    int KDefaultRange = 40;
}

CLIARandomSelector::CLIARandomSelector(Env *env, Z3IOExampleSpace *example_space): Z3Verifier(example_space) {
    auto* data = env->getConstRef(selector::KCLIARandomRangeName);
    if (data->isNull()) {
        env->setConst(selector::KCLIARandomRangeName, BuildData(Int, KDefaultRange));
    }
    KRandomRange = theory::clia::getIntValue(*data);
}

namespace {
    int _getRand(int n) {
        return rand() % (2 * n + 1) - n;
    }
}

bool CLIARandomSelector::verify(const FunctionContext &info, Example *counter_example) {
    if (counter_example) addExampleCount();
    if (!counter_example) return Z3Verifier::verify(info, counter_example);
    z3::solver s(ext->ctx);
    prepareZ3Solver(s, info);
    auto res = s.check();
    if (res == z3::unsat) return true;
    if (res != z3::sat) {
        LOG(FATAL) << "Z3 failed with " << res;
    }
    std::vector<z3::expr> int_param_list;
    for (int i = 0; i < example_space->type_list.size(); ++i) {
        if (example_space->type_list[i]->equal(theory::clia::getTInt().get())) {
            int_param_list.push_back(ext->buildVar(example_space->type_list[i].get(), "Param" + std::to_string(i)));
        }
    }
    std::random_shuffle(int_param_list.begin(), int_param_list.end());
    auto model = s.get_model();
    for (const auto& param: int_param_list) {
        int k = _getRand(KRandomRange);
        for (int _ = 0; _ < 5; _++) {
            s.push();
            if (rand() & 1) {
                s.add(param <= ext->ctx.int_val(k));
            } else {
                s.add(param >= ext->ctx.int_val(k));
            }
            res = s.check();
            if (res != z3::sat) {
                s.pop();
                continue;
            }
            model = s.get_model();
            break;
        }
    }
    getExample(model, counter_example);
    return false;
}