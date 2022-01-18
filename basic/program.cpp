//
// Created by pro on 2021/12/4.
//

#include "istool/basic/program.h"
#include "glog/logging.h"

Program::Program(const PSemantics &_semantics, const ProgramList &_sub_list):
    sub_list(_sub_list), semantics(_semantics) {
}
int Program::size() const {
    int res = 1;
    for (const auto& sub: sub_list) {
        res += sub->size();
    }
    return res;
}
Data Program::run(ExecuteInfo *info) const {
    return semantics->run(sub_list, info);
}
std::string Program::toString() const {
    auto res = semantics->getName();
    if (sub_list.empty()) return res;
    for (int i = 0; i < sub_list.size(); ++i) {
        if (i) res += ","; else res += "(";
        res += sub_list[i]->toString();
    }
    return res + ")";
}

PProgram program::buildConst(const Data &w) {
    ProgramList sub_list;
    return std::make_shared<Program>(semantics::buildConstSemantics(w), std::move(sub_list));
}

PProgram program::buildParam(int id, const PType &type) {
    ProgramList sub_list;
    return std::make_shared<Program>(semantics::buildParamSemantics(id, type), std::move(sub_list));
}

namespace {
    PProgram _programMap(Program* p, const ProgramConstructor& c, std::unordered_map<Program*, PProgram>& cache) {
        if (cache.find(p) != cache.end()) return cache[p];
        ProgramList new_subs;
        for (const auto& sub: p->sub_list) new_subs.push_back(_programMap(sub.get(), c, cache));
        return cache[p] = c(p->semantics, new_subs);
    }
}

PProgram program::programMap(Program *p, const ProgramConstructor &c) {
    std::unordered_map<Program*, PProgram> cache;
    return _programMap(p, c, cache);
}

PProgram program::rewriteParam(const PProgram &p, const ProgramList &param_list) {
    auto* ps = dynamic_cast<ParamSemantics*>(p->semantics.get());
    if (ps) {
        if (param_list[ps->id]) return param_list[ps->id];
        return p;
    }
    ProgramList sub_list;
    for (const auto& sub: p->sub_list) {
        sub_list.push_back(rewriteParam(sub, param_list));
    }
    return std::make_shared<Program>(p->semantics, sub_list);
}