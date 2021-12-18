//
// Created by pro on 2021/12/3.
//

#include "semantics.h"
#include "program.h"
#include <map>
#include "glog/logging.h"

Semantics::Semantics(const std::string &_name): name(_name) {
}
std::string Semantics::getName() {
    return name;
}

IOExecuteInfo::IOExecuteInfo(const DataList &_param_value): param_value(_param_value) {}

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
    auto* ioe = dynamic_cast<IOExecuteInfo*>(info);
#ifdef DEBUG
    assert(ioe);
#endif
    return ioe->param_value[id];
}

ConstSemantics::ConstSemantics(const Data &_w): FullExecutedSemantics(_w.toString()), w(_w) {
}
Data ConstSemantics::run(DataList&& inp_list, ExecuteInfo *info) {
    return w;
}

namespace {
    std::string buildDummyRecordName(const std::vector<std::string>& name_list) {
        std::string res = "[";
        for (int i = 0; i < name_list.size(); ++i) {
            if (i) res += ",";
            res += name_list[i];
        }
        return res;
    }
}

DummyRecordSemantics::DummyRecordSemantics(const std::vector<std::string> &_name_list):
    Semantics(buildDummyRecordName(_name_list)), name_list(_name_list) {
}
Data DummyRecordSemantics::run(const std::vector<std::shared_ptr<Program> > &sub_list, ExecuteInfo *info) {
    LOG(FATAL) << "Dummy Record Semantics cannot be executed";
}

namespace {
    std::map<std::string, PSemantics> semantics_pool;
}

PSemantics semantics::string2Semantics(const std::string &name) {
    if (semantics_pool.find(name) == semantics_pool.end()) {
        LOG(FATAL) << "Unknown semantics " << name;
    }
    return semantics_pool[name];
}

void semantics::registerSemantics(const std::string &name, const PSemantics &semantics) {
    if (semantics_pool.find(name) != semantics_pool.end()) {
        LOG(FATAL) << "Duplicated semantics " << name;
    }
    semantics_pool[name] = semantics;
}

PSemantics semantics::buildConstSemantics(const Data &w) {
    return std::make_shared<ConstSemantics>(w);
}

PSemantics semantics::buildParamSemantics(int id, PType &&type) {
    if (!type) {
        auto var_type = std::make_shared<TVar>("a");
        return std::make_shared<ParamSemantics>(std::move(var_type), id);
    }
    return std::make_shared<ParamSemantics>(std::move(type), id);
}