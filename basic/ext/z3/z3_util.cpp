//
// Created by pro on 2021/12/7.
//

#include "z3_util.h"
#include "z3_semantics.h"
#include "semantics/clia.h"
#include "glog/logging.h"
#include <map>

z3::expr ext::encodeZ3ExprForSemantics(const PSemantics &semantics, const z3::expr_vector &inp_list, const z3::expr_vector &param_list) {
    auto* cs = dynamic_cast<ConstSemantics*>(semantics.get());
    if (cs) {
        auto& value = cs->w.value;
        auto* z3v = dynamic_cast<Z3Value*>(value.get());
        if (!z3v) {
            LOG(FATAL) << "Ext z3: unsupported value " << value->toString();
        }
        return z3v->getZ3Expr(param_list.ctx());
    }
    auto* ps = dynamic_cast<ParamSemantics*>(semantics.get());
    if (ps) {
        return param_list[ps->id];
    }
    auto z3s = getZ3Semantics(semantics->name);
    return z3s->encodeZ3Expr(inp_list);
}

z3::expr ext::encodeZ3ExprForProgram(const PProgram &program, const z3::expr_vector &param_list) {
    z3::expr_vector sub_list(param_list.ctx());
    for (const auto& sp: program->sub_list) {
        sub_list.push_back(encodeZ3ExprForProgram(sp, param_list));
    }
    return encodeZ3ExprForSemantics(program->semantics, sub_list, param_list);
}

namespace {
    std::map<std::string, PZ3Semantics> z3_semantics_map;
}

PZ3Semantics ext::getZ3Semantics(const std::string &name) {
    if (z3_semantics_map.find(name) == z3_semantics_map.end()) {
        LOG(FATAL) << "Ext z3: unknown semantics " << name;
    }
    return z3_semantics_map[name];
}

void ext::registerZ3Semantics(const std::string &name, PZ3Semantics&& z3s) {
    if (z3_semantics_map.find(name) != z3_semantics_map.end()) {
        LOG(FATAL) << "Ext z3: duplicated semantics " << name;
    }
    z3_semantics_map[name] = z3s;
}

void ext::loadZ3Theory(TheoryToken token) {
    if (token == TheoryToken::CLIA) {
        clia::loadZ3SemanticsForCLIA();
    }
}

Data ext::getValueFromModel(const z3::expr &expr, const z3::model &model, bool is_strict) {
    auto value = model.eval(expr);
    if (expr.is_bool()) {
        bool w = true;
        if (value.is_bool()) {
            w = value.bool_value() == Z3_L_TRUE;
        } else if (is_strict) return Data();
        return Data(std::make_shared<BoolValue>(w));
    }
    if (expr.is_int()) {
        int w = 0;
        if (value.is_numeral()) {
            w = value.get_numeral_int();
        } else if (is_strict) return Data();
        return Data(std::make_shared<IntValue>(w));
    }
    LOG(FATAL) << "Fail in parsing expr " << expr << " with sort " << expr.get_sort();
}

z3::expr ext::buildVar(const PType &type, const std::string &name, z3::context& ctx) {
    auto* z3t = dynamic_cast<Z3Type*>(type.get());
    return z3t->buildVar(name, ctx);
}

z3::expr ext::buildConst(const Data &data, z3::context &ctx) {
    auto* z3v = dynamic_cast<Z3Value*>(data.get());
    return z3v->getZ3Expr(ctx);
}

z3::context ext::z3_ctx;