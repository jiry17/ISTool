//
// Created by pro on 2021/12/3.
//

#include "istool/basic/semantics.h"
#include "istool/basic/program.h"
#include "istool/basic/env.h"
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

NormalSemantics::NormalSemantics(const std::string& name, const PType &_oup_type, const TypeList &_inp_list):
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

std::string FunctionContext::toString() const {
    if (empty()) return "{}";
    std::string res = "{";
    for (const auto& info: *this) {
        res += info.first + ":" + info.second->toString() + ";";
    }
    res[res.length() - 1] = '}';
    return res;
}

InvokeSemantics::InvokeSemantics(const std::string &_func_name, const PType &oup_type, const TypeList &inp_list, Env* _env):
    NormalSemantics(_func_name, std::move(oup_type), std::move(inp_list)), env(_env) {
}

Data InvokeSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    auto p = info->func_context[name];
    if (!p) throw SemanticsError();
    return env->run(p.get(), inp_list);
}

#define TBOOL type::getTBool()

NotSemantics::NotSemantics(): NormalSemantics("!", TBOOL, {TBOOL}) {
}
Data NotSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    return Data(std::make_shared<BoolValue>(!inp_list[0].isTrue()));
}

AndSemantics::AndSemantics(): Semantics("&&"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data AndSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    auto x = sub_list[0]->run(info);
    if (!x.isTrue()) return x;
    return sub_list[1]->run(info);
}

OrSemantics::OrSemantics(): Semantics("||"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data OrSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    auto x = sub_list[0]->run(info);
    if (x.isTrue()) return x;
    return sub_list[1]->run(info);
}

ImplySemantics::ImplySemantics(): Semantics("=>"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data ImplySemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    auto x = sub_list[0]->run(info);
    if (!x.isTrue()) return Data(std::make_shared<BoolValue>(true));
    return sub_list[1]->run(info);
}

void semantics::loadLogicSemantics(Env* env) {
    LoadSemantics("=>", Imply); LoadSemantics("!", Not);
    LoadSemantics("&&", And); LoadSemantics("||", Or);
    LoadSemantics("and", And); LoadSemantics("or", Or);
    LoadSemantics("not", Not);
}

FunctionContext semantics::buildSingleContext(const std::string &name, const std::shared_ptr<Program> &program) {
    FunctionContext res; res[name] = program;
    return res;
}