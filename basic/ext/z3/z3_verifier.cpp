//
// Created by pro on 2021/12/7.
//

#include "z3_verifier.h"
#include "z3_util.h"
#include <map>
#include "glog/logging.h"

Z3CallInfo::Z3CallInfo(const std::string &_name, const z3::expr_vector &_inp_symbols, const z3::expr &_oup_symbol):
    name(_name), inp_symbols(_inp_symbols), oup_symbol(_oup_symbol){
}

Z3IOVerifier::Z3IOVerifier(const z3::expr_vector &_cons_list, const Z3CallInfo& _info):
    cons_list(_cons_list), info(_info) {
}

PExample Z3IOVerifier::verify(const PProgram& program) const {
    z3::solver solver(cons_list.ctx());
    solver.push();
    solver.add(!z3::mk_and(cons_list));
    solver.add(ext::encodeZ3ExprForProgram(program, info.inp_symbols) == info.oup_symbol);
    auto res = solver.check();
    if (res == z3::unsat) return nullptr;
    DataList inp_list;
    auto model = solver.get_model();
    for (const auto& inp_symbol: info.inp_symbols) {
        inp_list.push_back(ext::getValueFromModel(inp_symbol, model));
    }
    solver.pop();
    for (int i = 0; i < info.inp_symbols.size(); ++i) {
        auto* z3v = dynamic_cast<Z3Value*>(inp_list[i].get());
        assert(z3v);
        solver.add(info.inp_symbols[i] == z3v->getZ3Expr(cons_list.ctx()));
    }
    solver.add(z3::mk_and(cons_list));
    assert(solver.check() == z3::sat);
    auto new_model = solver.get_model();
    auto oup = ext::getValueFromModel(info.oup_symbol, new_model);
    return std::make_shared<IOExample>(std::move(inp_list), std::move(oup));
}