//
// Created by pro on 2022/1/15.
//

#include "istool/ext/deepcoder/higher_order_operator.h"
#include "istool/ext/deepcoder/anonymous_function.h"
#include "istool/basic/env.h"
#include "glog/logging.h"

ApplySemantics::ApplySemantics(): Semantics("apply") {}
Data ApplySemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    auto s = ext::ho::getSemantics(sub_list[0]->run(info));
    ProgramList sub;
    for (int i = 1; i < sub_list.size(); ++i) sub.push_back(sub_list[i]);
    return s->run(sub, info);
}

CurrySemantics::CurrySemantics(): Semantics("curry") {}
Data CurrySemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    assert(sub_list.size() == 1);
    auto p = sub_list[0];
    auto f = [p](const ProgramList& extra_sub_list, ExecuteInfo* _info) {
        auto full_sub_list = p->sub_list;
        for (const auto& extra: extra_sub_list) full_sub_list.push_back(extra);
        return p->semantics->run(full_sub_list, _info);
    };
    return ext::ho::buildAnonymousData(f);
}

TmpExecuteInfo::TmpExecuteInfo(const DataList &param_value): ExecuteInfo(param_value) {}
void TmpExecuteInfo::clear(const std::string &name) {
    auto it = tmp_value_map.find(name);
    assert(it != tmp_value_map.end());
    tmp_value_map.erase(it);
}
Data TmpExecuteInfo::get(const std::string &name) {
    auto it = tmp_value_map.find(name);
    assert(it != tmp_value_map.end());
    return it->second;
}
void TmpExecuteInfo::set(const std::string &name, const Data &value) {
    auto it = tmp_value_map.find(name);
    assert(it == tmp_value_map.end());
    tmp_value_map[name] = value;
}

TmpSemantics::TmpSemantics(const std::string &_name): FullExecutedSemantics(_name) {}
Data TmpSemantics::run(DataList &&inp, ExecuteInfo *info) {
    auto* ti = dynamic_cast<TmpExecuteInfo*>(info);
    if (!ti) {
        LOG(FATAL) << "TmpSemantics require TmpExecuteInfo";
    }
    return ti->get(name);
}

namespace {
    std::string _getLambdaName(const std::vector<std::string>& name_list) {
        std::string res("lambda ");
        for (int i = 0; i < name_list.size(); ++i) {
            if (i) res += ","; res += name_list[i];
        }
        return res + ".";
    }
}

LambdaSemantics::LambdaSemantics(const std::vector<std::string> &_name_list):
    Semantics(_getLambdaName(_name_list)), name_list(_name_list) {
}
Data LambdaSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    assert(sub_list.size() == 1);
    auto p = sub_list[0];
    auto tmp_list = name_list;
    auto f = [p, tmp_list](const ProgramList& extra_sub_list, ExecuteInfo* _info) {
        assert(extra_sub_list.size() == tmp_list.size());
        auto* ti = dynamic_cast<TmpExecuteInfo*>(_info);
        if (!ti) {
            LOG(FATAL) << "TmpSemantics require TmpExecuteInfo";
        }
        DataList tmp_values;
        for (const auto& sub: extra_sub_list) tmp_values.push_back(sub->run(_info));
        for (int i = 0; i < tmp_values.size(); ++i) ti->set(tmp_list[i], tmp_values[i]);
        auto res = p->run(ti);
        for (int i = 0; i < tmp_values.size(); ++i) ti->clear(tmp_list[i]);
        return res;
    };
    return ext::ho::buildAnonymousData(f);
}

void ext::ho::loadHigherOrderOperators(Env *env) {
    LoadSemantics("apply", Apply); LoadSemantics("curry", Curry);
    // TODO: change the type of executeInfo
}