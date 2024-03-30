//
// Created by zyw on 2024/3/29.
//

#ifndef ISTOOL_INCRE_DP_H
#define ISTOOL_INCRE_DP_H

#include "incre_program.h"
#include "incre_syntax.h"
#include "incre_types.h"

namespace incre::syntax {
    class IncreCommandWalker {
    protected:
        // program context, used in getting type of a term
        IncreFullContext ctx;
        // type checker
        incre::types::IncreTypeChecker* checker;

        virtual void walkThroughTerm(Term term) = 0;
        virtual void initialize(Command command) {}
        virtual void preProcess(Term term) {}
        virtual void postProcess(Term term) {}
    public:
        void walkThrough(Command command);
        IncreCommandWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : ctx(_ctx), checker(_checker) {}
        virtual ~IncreCommandWalker() = default;
    };

    class DpCommandWalker : public IncreCommandWalker {
    protected:
        virtual void walkThroughTerm(Term term);
        virtual void initialize(Command command) {}
        virtual void preProcess(Term term) {}
        virtual void postProcess(Term term) {}
    public:
        // store the type of partial solution, which is the result of DpProgramWalker
        Ty res = nullptr;
        // already has result
        bool has_res = false;
        void updateRes(Ty new_res);
        DpCommandWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : IncreCommandWalker(_ctx, _checker) {}
        virtual ~DpCommandWalker() = default;
    };
    
    class DpProgramWalker: public IncreProgramWalker {
    protected:
        virtual void visit(CommandDef* command);
        virtual void visit(CommandBindTerm* command);
        virtual void visit(CommandDeclare* command);
        virtual void initialize(IncreProgramData* program);
    public:
        // command walker
        DpCommandWalker* cmdWalker;
        DpProgramWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : cmdWalker(new DpCommandWalker(_ctx, _checker)) {}
        ~DpProgramWalker() = default;
    };

    // get the type of partial solution using DpProgramWalker
    Ty getSolutionType(IncreProgramData* program, IncreFullContext ctx);
}

namespace grammar {
    // post process of DP program list
    void postProcessDpProgramList(ProgramList& program_list);
    // check whether this program is relative symmetry
    bool postProcessBoolProgram(PProgram program);
    // post process of BOOL program list
    void postProcessBoolProgramList(ProgramList& program_list);
    // merge DP program and BOOL program to get the full program
    ProgramList mergeProgramList(ProgramList& dp_program_list, ProgramList& bool_program_list);
}

#endif //ISTOOL_INCRE_DP_H