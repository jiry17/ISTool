//
// Created by pro on 2022/1/12.
//

#include "istool/selector/split/z3_split_selector.h"
#include "istool/selector/selector.h"
#include "istool/solver/enum/enum_util.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "glog/logging.h"

Z3SplitSelector::Z3SplitSelector(Specification *spec, int n): SplitSelector(spec->info_list[0], n), info(spec->info_list[0]),
    Z3Verifier(dynamic_cast<Z3ExampleSpace*>(spec->example_space.get())) {
    io_space = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "Z3SplitSelector requires Z3IOExampleSpace";
    }
    time_out = selector::getSelectorTimeOut(spec->env.get());
}

bool Z3SplitSelector::verify(const FunctionContext &res, Example *counter_example) {
    if (counter_example) addExampleCount();
    if (!counter_example) return Z3Verifier::verify(res, counter_example);
    z3::solver solver(ext->ctx);
    prepareZ3Solver(solver, res);
    auto z3_res = solver.check();
    if (z3_res == z3::unsat) return true;
    if (z3_res != z3::sat) LOG(FATAL) << "Z3 failed with result " << z3_res;
    z3::model model = solver.get_model();

    z3::expr_vector param_list(ext->ctx);
    for (int i = 0; i < example_space->type_list.size(); ++i) {
        param_list.push_back(ext->buildVar(example_space->type_list[i].get(), "Param" + std::to_string(i)));
    }
    z3::expr_vector inp_list(ext->ctx);
    for (int i = 0; i < io_space->inp_list.size(); ++i) {
        auto var = ext->buildVar(info->inp_type_list[i].get(), "Input" + std::to_string(i));
        auto encode_res = ext->encodeZ3ExprForProgram(io_space->inp_list[i].get(), param_list);
        if (!encode_res.cons_list.empty()) {
            LOG(FATAL) << "Z3SplitSelector require the input program always to be valid";
        }
        inp_list.push_back(var);
        solver.add(var == encode_res.res);
    }
    z3::expr_vector seed_oup_list(ext->ctx), seed_valid_list(ext->ctx);
    for (int i = 0; i < seed_list.size(); ++i) {
        auto oup_var = ext->buildVar(info->oup_type.get(), "Output" + std::to_string(i));
        auto encode_res = ext->encodeZ3ExprForProgram(seed_list[i].get(), inp_list);
        seed_oup_list.push_back(oup_var); solver.add(oup_var == encode_res.res);
        if (encode_res.cons_list.empty()) {
            seed_valid_list.push_back(ext->ctx.bool_val(true));
        } else {
            auto valid_var = ext->ctx.bool_const(("Valid" + std::to_string(i)).c_str());
            seed_valid_list.push_back(valid_var);
            solver.add(valid_var == z3::mk_and(encode_res.cons_list));
        }
    }

    auto group_size = ext->ctx.int_const("group-size");
    for (int i = 0; i + 1 < seed_list.size(); ++i) {
        z3::expr_vector eq_list(ext->ctx);
        for (int j = i + 1; j < seed_list.size(); ++j)
            eq_list.push_back(z3::ite(seed_oup_list[i] == seed_oup_list[j] && seed_valid_list[j],
                    ext->ctx.int_val(1), ext->ctx.int_val(0)));
        solver.add(z3::implies(seed_valid_list[i], z3::sum(eq_list) <= group_size));
    }


    auto* tmp_guard = new TimeGuard(theory::clia::getIntValue(*time_out) / 1000.0);
    int l = 0, r = seed_list.size();
    while (l < r) {
        int mid = l + r >> 1;
        ext->setTimeOut(solver, tmp_guard);
        solver.push();
        solver.add(group_size <= ext->ctx.int_val(mid));
        z3_res = solver.check();
        if (z3_res == z3::sat) {
            model = solver.get_model();
            r = mid;
        } else {
            solver.pop();
            l = mid + 1;
        }
    }

    getExample(model, counter_example);
    return false;
}

