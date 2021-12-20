//
// Created by pro on 2021/12/3.
//

#include "istool/basic/semantics.h"
#include "istool/basic/program.h"
#include <map>
#include "glog/logging.h"

Semantics::Semantics(const std::string &_name): name(_name) {
}
std::string Semantics::getName() {
    return name;
}

FullExecutedSemantics::FullExecutedSemantics(const std::string &name): Semantics(name) {}
Data FullExecutedSemantics::run(const std::vector<std::shared_ptr<Program>> &sub_list, ExecuteInfo *info) {
    DataList res;
    for (const auto& p: sub_list) res.push_back(p->run(info));
    return run(std::move(res), info);
}

NormalSemantics::NormalSemantics(const std::string& name, PType &&_oup_type, TypeList &&_inp_list):
    TypedSemantics(std::move(_oup_type), std::move(_inp_list)), FullExecutedSemantics(name) {
}

ParamSemantics::ParamSemantics(PType &&type, int _id):
        NormalSemantics("Param" + std::to_string(_id), std::move(type), {}), id(_id) {
}
Data ParamSemantics::run(DataList&& inp_list, ExecuteInfo *info) {
    return info->param_value[id];
}

ConstSemantics::ConstSemantics(const Data &_w): FullExecutedSemantics(_w.toString()), w(_w) {
}
Data ConstSemantics::run(DataList&& inp_list, ExecuteInfo *info) {
    return w;
}

PSemantics semantics::buildConstSemantics(const Data &w) {
    return std::make_shared<ConstSemantics>(w);
}

PSemantics semantics::buildParamSemantics(int id, const PType &type) {
    if (!type) {
        auto var_type = std::make_shared<TVar>("a");
        return std::make_shared<ParamSemantics>(std::move(var_type), id);
    }
    PType tmp = type;
    return std::make_shared<ParamSemantics>(std::move(tmp), id);
}

FunctionContextInfo::FunctionContextInfo(const DataList &_param, const FunctionContext &_context):
    ExecuteInfo(_param), context(_context) {
}
std::shared_ptr<Program> FunctionContextInfo::getFunction(const std::string &name) const {
    if (context.find(name) == context.end()) {
        return nullptr;
    }
    return context.find(name)->second;
}

InvokeSemantics::InvokeSemantics(const std::string &_func_name, PType &&oup_type, TypeList &&inp_list):
    NormalSemantics(_func_name, std::move(oup_type), std::move(inp_list)) {
}

Data InvokeSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    auto* f_info = dynamic_cast<FunctionContextInfo*>(info);
    auto p = f_info->getFunction(name);
    if (!p) throw SemanticsError();
    return program::run(p, inp_list);
}