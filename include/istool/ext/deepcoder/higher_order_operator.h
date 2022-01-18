//
// Created by pro on 2022/1/15.
//

#ifndef ISTOOL_HIGHER_ORDER_OPERATOR_H
#define ISTOOL_HIGHER_ORDER_OPERATOR_H

#include "istool/basic/program.h"

class ApplySemantics: public Semantics {
public:
    ApplySemantics();
    virtual ~ApplySemantics() = default;
    virtual Data run(const ProgramList& sub_list, ExecuteInfo* info);
};

class CurrySemantics: public Semantics {
public:
    CurrySemantics();
    virtual ~CurrySemantics() = default;
    virtual Data run(const ProgramList& sub_list, ExecuteInfo* info);
};

class TmpExecuteInfo: public ExecuteInfo {
public:
    std::unordered_map<std::string, Data> tmp_value_map;
    TmpExecuteInfo(const DataList& param_value);
    void set(const std::string& name, const Data& value);
    void clear(const std::string& name);
    Data get(const std::string& name);
};

class TmpSemantics: public FullExecutedSemantics {
public:
    TmpSemantics(const std::string& _name);
    virtual Data run(DataList&& inp, ExecuteInfo* info);
    virtual ~TmpSemantics() = default;
};

class LambdaSemantics: public Semantics {
public:
    std::vector<std::string> name_list;
    LambdaSemantics(const std::vector<std::string>& name_list);
    virtual Data run(const ProgramList& sub_list, ExecuteInfo* info);
    virtual ~LambdaSemantics() = default;
};

namespace ext::ho {
    void loadHigherOrderOperators(Env* env);
}

#endif //ISTOOL_HIGHER_ORDER_OPERATOR_H
