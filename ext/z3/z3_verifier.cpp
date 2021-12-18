//
// Created by pro on 2021/12/7.
//

#include "z3_verifier.h"
#include "z3_extension.h"
#include <map>
#include "glog/logging.h"

Z3Info::Z3Info(const z3::expr &_expr, PType &_type): expr(_expr), type(_type) {
}

Z3CallInfo::Z3CallInfo(const std::string &_name, const Z3InfoList &_inp_symbols, const Z3Info &_oup_symbol):
    inp_infos(_inp_symbols), oup_info(_oup_symbol), name(_name) {
}

z3::expr_vector Z3CallInfo::getInpList() const {
    z3::expr_vector inp_list(oup_info.expr.ctx());
    for (auto& info: inp_infos) {
        inp_list.push_back(info.expr);
    }
    return inp_list;
}

Z3IOVerifier::Z3IOVerifier(const z3::expr_vector &_cons_list, const Z3CallInfo& _info, Env* env):
    cons_list(_cons_list), info(_info), z3_env(ext::z3::getZ3Extension(env)) {
}

PExample Z3IOVerifier::verify(const PProgram& program) const {
    z3::solver solver(cons_list.ctx());
    solver.push();
    solver.add(!z3::mk_and(cons_list));
    solver.add(z3_env->encodeZ3ExprForProgram(program.get(), info.getInpList()));
    auto res = solver.check();
    if (res == z3::unsat) return nullptr;
    DataList inp_list;
    auto model = solver.get_model();
    for (const auto& info: info.inp_infos) {
        inp_list.push_back(z3_env->getValueFromModel(model, info.expr, info.type.get()));
    }
    solver.pop();
    for (int i = 0; i < info.inp_infos.size(); ++i) {
        solver.add(z3_env->buildConst(inp_list[i]) == info.inp_infos[i].expr);
    }
    solver.add(z3::mk_and(cons_list));
    assert(solver.check() == z3::sat);
    auto new_model = solver.get_model();
    auto oup = z3_env->getValueFromModel(new_model, info.oup_info.expr, info.oup_info.type.get());
    return std::make_shared<IOExample>(std::move(inp_list), std::move(oup));
}