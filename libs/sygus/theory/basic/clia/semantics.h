//
// Created by pro on 2021/12/19.
//

#ifndef ISTOOL_SEMANTICS_H
#define ISTOOL_SEMANTICS_H

class IntPlusSemantics : public NormalSemantics {
public:
    IntPlusSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntPlusSemantics() = default;
};

class IntMinusSemantics : public NormalSemantics {
public:
    IntMinusSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntMinusSemantics() = default;
};

class IntTimesSemantics : public NormalSemantics {
public:
    IntTimesSemantics();
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

class IntLqSemantics : public NormalSemantics {
public:
    IntLqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntLqSemantics() = default;
};

class IntGqSemantics : public NormalSemantics {
public:
    IntGqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntGqSemantics() = default;
};

class IntLeqSemantics : public NormalSemantics {
public:
    IntLeqSemantics();
    virtual Data run(DataList &&inp_list, ExecuteInfo *info);
    ~IntLeqSemantics() = default;
};

class IntGeqSemantics : public NormalSemantics {
public:
    IntGeqSemantics();
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

#endif //ISTOOL_SEMANTICS_H
