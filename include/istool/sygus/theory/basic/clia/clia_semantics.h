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

DefineNormalSemantics(IntDiv)
DefineNormalSemantics(IntMod)
DefineNormalSemantics(Lq)
DefineNormalSemantics(Gq)
DefineNormalSemantics(Leq)
DefineNormalSemantics(Geq)
DefineNormalSemantics(Eq)

class IteSemantics : public Semantics, public TypedSemantics {
public:
    IteSemantics();
    virtual Data run(const ProgramList &sub_list, ExecuteInfo *info);
    ~IteSemantics() = default;
};

namespace theory {
    namespace clia {
        extern const std::string KINFName;
    }
    void loadCLIASemantics(Env* env);
}

#endif //ISTOOL_CLIA_SEMANTICS_H
