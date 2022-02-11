//
// Created by pro on 2022/1/12.
//

#include "istool/selector/split/z3_splitor.h"
#include "istool/selector/selector.h"
#include "istool/solver/enum/enum_util.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "glog/logging.h"

Z3Splitor::Z3Splitor(ExampleSpace *_example_space, const PType &_oup_type, const TypeList &_inp_type_list):
    Splitor(_example_space), env(_example_space->env), oup_type(_oup_type), inp_type_list(_inp_type_list) {
    io_space = dynamic_cast<Z3IOExampleSpace*>(example_space);
    if (!io_space) {
        LOG(FATAL) << "Z3Splitor requires Z3IOExampleSpace";
    }
    ext = ext::z3::getExtension(env);
}

bool Z3Splitor::getSplitExample(Program *cons_program, const FunctionContext &info, const ProgramList &seed_list,
        Example *counter_example, TimeGuard *guard) {
    z3::solver solver(ext->ctx);

    z3::expr_vector param_list(ext->ctx);
    for (int i = 0; i < io_space->type_list.size(); ++i) {
        param_list.push_back(ext->buildVar(io_space->type_list[i].get(), "Param" + std::to_string(i)));
    }
    z3::expr_vector inp_list(ext->ctx);
    for (int i = 0; i < io_space->inp_list.size(); ++i) {
        auto var = ext->buildVar(inp_type_list[i].get(), "Input" + std::to_string(i));
        auto encode_res = ext->encodeZ3ExprForProgram(io_space->inp_list[i].get(), ext::z3::z3Vector2EncodeList(param_list));
        if (!encode_res.cons_list.empty()) {
            LOG(FATAL) << "Z3SplitSelector require the input program always to be valid";
        }
        inp_list.push_back(var);
        solver.add(var == encode_res.res);
    }
    auto global_oup_var = ext->buildVar(oup_type.get(), "Output");
    auto full_inp_list = inp_list;
    full_inp_list.push_back(global_oup_var);
    auto oup_cons_res = ext->encodeZ3ExprForProgram(io_space->oup_cons.get(), ext::z3::z3Vector2EncodeList(full_inp_list));
    solver.add(oup_cons_res.res && z3::mk_and(oup_cons_res.cons_list));
    auto cons_res = ext->encodeZ3ExprForConsProgram(cons_program, info, ext::z3::z3Vector2EncodeList(full_inp_list));
    solver.add(oup_cons_res.res && z3::mk_and(cons_res.cons_list));

    auto z3_res = solver.check();
    if (z3_res == z3::unsat) return true;
    if (z3_res != z3::sat) LOG(FATAL) << "Z3 failed with result " << z3_res;
    if (!counter_example) return false;
    z3::model model = solver.get_model();

    z3::expr_vector seed_oup_list(ext->ctx), seed_valid_list(ext->ctx);
    for (int i = 0; i < seed_list.size(); ++i) {
        auto oup_var = ext->buildVar(oup_type.get(), "Output" + std::to_string(i));
        auto encode_res = ext->encodeZ3ExprForProgram(seed_list[i].get(), ext::z3::z3Vector2EncodeList(inp_list));
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

    int l = 0, r = seed_list.size();
    while (l < r) {
        int mid = l + r >> 1;
        ext->setTimeOut(solver, guard);
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

    counter_example->clear();
    for (int i = 0; i < param_list.size(); ++i) {
        counter_example->push_back(ext->getValueFromModel(model, param_list[i], io_space->type_list[i].get(), false));
    }
    return false;
}