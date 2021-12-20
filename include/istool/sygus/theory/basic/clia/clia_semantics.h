//
// Created by pro on 2021/12/19.
//

#ifndef ISTOOL_CLIA_SEMANTICS_H
#define ISTOOL_CLIA_SEMANTICS_H

#include "istool/basic/program.h"
#include "istool/basic/env.h"

class IntPlusSemantics : public NormalSemantics {
public:
    Data* inf;
    IntPlusSemantics(Data* _inf);
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntPlusSemantics() = default;
};

class IntMinusSemantics : public NormalSemantics {
public:
    Data* inf;
    IntMinusSemantics(Data* _inf);
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntMinusSemantics() = default;
};

class IntTimesSemantics : public NormalSemantics {
public:
    Data* inf;
    IntTimesSemantics(Data* _inf);
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntTimesSemantics() = default;
};

class IntDivSemantics : public NormalSemantics {
public:
    IntDivSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntDivSemantics() = default;
};

class IntModSemantics : public NormalSemantics {
public:
    IntModSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntModSemantics() = default;
};

class LqSemantics : public NormalSemantics {
public:
    LqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~LqSemantics() = default;
};

class GqSemantics : public NormalSemantics {
public:
    GqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~GqSemantics() = default;
};

class LeqSemantics : public NormalSemantics {
public:
    LeqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~LeqSemantics() = default;
};

class GeqSemantics : public NormalSemantics {
public:
    GeqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
};

class EqSemantics: public NormalSemantics {
public:
    EqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo* info);
};

class NotSemantics : public NormalSemantics {
public:
    NotSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~NotSemantics() = default;
};

class AndSemantics : public Semantics, public TypedSemantics {
public:
    AndSemantics();
    virtual Data run(const ProgramList &sub_list, ExecuteInfo *info);
    ~AndSemantics() = default;
};

class OrSemantics : public Semantics, public TypedSemantics {
public:
    OrSemantics();
    virtual Data run(const ProgramList &sub_list, ExecuteInfo *info);
    ~OrSemantics() = default;
};

class ImplySemantics: public Semantics, public TypedSemantics {
public:
    ImplySemantics();
    virtual Data run(const ProgramList& sub_list, ExecuteInfo* info);
};

class IteSemantics : public Semantics, public TypedSemantics {
public:
    IteSemantics();
    virtual Data run(const ProgramList &sub_list, ExecuteInfo *info);
    ~IteSemantics() = default;
};

namespace theory {
    void loadCLIASemantics(Env* env);
}

#endif //ISTOOL_CLIA_SEMANTICS_H
