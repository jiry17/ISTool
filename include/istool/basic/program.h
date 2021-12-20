//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_PROGRAM_H
#define ISTOOL_PROGRAM_H

#include "semantics.h"

class Program;
typedef std::shared_ptr<Program> PProgram;
typedef std::vector<PProgram> ProgramList;

class Program {
public:
    PSemantics semantics;
    ProgramList sub_list;
    Program(PSemantics&& _semantics, const ProgramList& _sub_list);
    int size() const;
    Data run(ExecuteInfo* info) const;
    std::string toString() const;
    virtual ~Program() = default;
};

namespace program {
    PProgram buildParam(int id, const PType& type = nullptr);
    PProgram buildConst(const Data& w);
    Data run(const PProgram& program, const DataList& inp);
    Data runWithFunc(const PProgram& program, const DataList& inp, const FunctionContext& ctx);
}

#endif //ISTOOL_PROGRAM_H
