//
// Created by pro on 2021/12/4.
//

#include "program.h"
#include "glog/logging.h"

Program::Program(PSemantics &&_semantics, const ProgramList &_sub_list):
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

PProgram program::buildParam(int id, PType &&type) {
    ProgramList sub_list;
    return std::make_shared<Program>(semantics::buildParamSemantics(id, std::move(type)), std::move(sub_list));
}

Data program::run(const PProgram& program, const DataList &inp) {
    auto* log = new IOExecuteInfo(inp);
    auto res = program->run(log);
    delete log;
    return res;
}