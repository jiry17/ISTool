//
// Created by pro on 2021/12/3.
//

#ifndef ISTOOL_SEMANTICS_H
#define ISTOOL_SEMANTICS_H

#include <exception>
#include "data.h"
#include "z3++.h"
#include "type.h"

class ExecuteInfo {
public:
    virtual ~ExecuteInfo() = default;
};

class IOExecuteInfo: public ExecuteInfo {
public:
    DataList param_value;
    IOExecuteInfo(const DataList& _param_value);
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
    TypedSemantics(PType&& _oup_type, TypeList&& _inp_list): oup_type(_oup_type), inp_type_list(_inp_list) {}
};

class FullExecutedSemantics: public Semantics {
public:
    FullExecutedSemantics(const std::string& name);
    Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo* info);
    virtual Data run(DataList&& inp_list, ExecuteInfo* info) = 0;
};

class NormalSemantics: public FullExecutedSemantics, public TypedSemantics {
public:
    NormalSemantics(const std::string& name, PType&& _oup_type, TypeList&& _inp_list);
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

// let std::map<std::string, Program*> to be an instance of Program*
class DummyRecordSemantics: public Semantics {
public:
    std::vector<std::string> name_list;
    DummyRecordSemantics(const std::vector<std::string>& _name_list);
    virtual Data run(const std::vector<std::shared_ptr<Program>>& sub_list, ExecuteInfo* info);
    ~DummyRecordSemantics() = default;
};

namespace semantics {
    PSemantics buildParamSemantics(int id, PType&& type = nullptr);
    PSemantics buildConstSemantics(const Data& w);
    PSemantics string2Semantics(const std::string& name);
    void registerSemantics(const std::string& name, const PSemantics& semantics);
}


#endif //ISTOOL_SEMANTICS_H
