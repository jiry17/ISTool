//
// Created by pro on 2021/12/3.
//

#ifndef ISTOOL_SEMANTICS_H
#define ISTOOL_SEMANTICS_H

#include <exception>
#include <unordered_map>
#include "data.h"
#include "z3++.h"
#include "type.h"

class ExecuteInfo {
public:
    DataList param_value;
    ExecuteInfo(const DataList& _param_value);
    virtual ~ExecuteInfo() = default;
};

class Program;

struct SemanticsError: public std::exception {
};

class Semantics {
public:
    std::string name;
    Semantics(const std::string& _name);
    virtual Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo* info) = 0;
    virtual std::string getName();
    virtual ~Semantics() = default;
};

typedef std::shared_ptr<Semantics> PSemantics;

class TypedSemantics {
public:
    PType oup_type;
    TypeList inp_type_list;
    TypedSemantics(const PType& _oup_type, const TypeList& _inp_list): oup_type(_oup_type), inp_type_list(_inp_list) {}
};

class FullExecutedSemantics: public Semantics {
public:
    FullExecutedSemantics(const std::string& name);
    Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo* info);
    virtual Data run(DataList&& inp_list, ExecuteInfo* info) = 0;
};

class NormalSemantics: public FullExecutedSemantics, public TypedSemantics {
public:
    NormalSemantics(const std::string& name, const PType& _oup_type, const TypeList& _inp_list);
};

class ParamSemantics: public NormalSemantics {
public:
    int id;
    ParamSemantics(PType&& type, int _id);
    virtual Data run(DataList&&, ExecuteInfo* info);
    ~ParamSemantics() = default;
};

class ConstSemantics: public FullExecutedSemantics {
public:
    Data w;
    ConstSemantics(const Data& _w);
    virtual Data run(DataList&&, ExecuteInfo* info);
    ~ConstSemantics() = default;
};

class FunctionContext: public std::unordered_map<std::string, std::shared_ptr<Program>> {
public:
    std::string toString() const;
};

class FunctionContextInfo: public ExecuteInfo {
public:
    FunctionContext context;
    FunctionContextInfo(const DataList& _param, const FunctionContext& _context);
    std::shared_ptr<Program> getFunction(const std::string& name) const;
    virtual ~FunctionContextInfo() = default;
};

class InvokeSemantics: public NormalSemantics {
public:
    InvokeSemantics(const std::string& _func_name, const PType& oup_type, const TypeList& inp_list);
    virtual Data run(DataList&&, ExecuteInfo* info);
    virtual ~InvokeSemantics() = default;
};

#define DefineNormalSemantics(name) \
class name ## Semantics : public NormalSemantics { \
public: \
    name ## Semantics(); \
    virtual Data run(DataList &&inp_list, ExecuteInfo *info); \
    ~name ## Semantics() = default; \
};

// basic logic semantics
DefineNormalSemantics(Not)

class AndSemantics : public Semantics, public TypedSemantics {
public:
    AndSemantics();
    virtual Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo *info);
    ~AndSemantics() = default;
};

class OrSemantics : public Semantics, public TypedSemantics {
public:
    OrSemantics();
    virtual Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo *info);
    ~OrSemantics() = default;
};

class ImplySemantics: public Semantics, public TypedSemantics {
public:
    ImplySemantics();
    virtual Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo* info);
};

class Env;
#define LoadSemantics(name, sem) env->setSemantics(name, std::make_shared<sem ## Semantics>())

namespace semantics {
    void loadLogicSemantics(Env* env);
    PSemantics buildParamSemantics(int id, const PType& type = nullptr);
    PSemantics buildConstSemantics(const Data& w);
    FunctionContext buildSingleContext(const std::string& name, const std::shared_ptr<Program>& program);
}


#endif //ISTOOL_SEMANTICS_H
