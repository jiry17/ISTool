//
// Created by zyw on 2024/3/29.
//

#ifndef ISTOOL_INCRE_DP_H
#define ISTOOL_INCRE_DP_H

#include "incre_program.h"
#include "incre_syntax.h"
#include "incre_types.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include <set>

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

    class DpTypeCommandWalker : public IncreCommandWalker {
    protected:
        virtual void walkThroughTerm(Term term);
        virtual void initialize(Command command) {}
        virtual void preProcess(Term term) {}
        virtual void postProcess(Term term) {}
    public:
        // store the type of partial solution, which is the result of DpTypeProgramWalker
        Ty res = nullptr;
        // already has result
        bool has_res = false;
        void updateRes(Ty new_res);
        DpTypeCommandWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : IncreCommandWalker(_ctx, _checker) {}
        virtual ~DpTypeCommandWalker() = default;
    };
    
    class DpTypeProgramWalker: public IncreProgramWalker {
    protected:
        virtual void visit(CommandDef* command);
        virtual void visit(CommandBindTerm* command);
        virtual void visit(CommandDeclare* command);
        virtual void initialize(IncreProgramData* program);
    public:
        // command walker
        DpTypeCommandWalker* cmdWalker;
        DpTypeProgramWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : cmdWalker(new DpTypeCommandWalker(_ctx, _checker)) {}
        ~DpTypeProgramWalker() = default;
    };

    // get the type of partial solution using DpTypeProgramWalker
    Ty getSolutionType(IncreProgramData* program, IncreFullContext ctx);

    class DpObjCommandWalker : public IncreCommandWalker {
    protected:
        virtual void walkThroughTerm(Term term);
        virtual void initialize(Command command) {}
        virtual void preProcess(Term term) {}
        virtual void postProcess(Term term) {}
    public:
        // store the type of partial solution, which is the result of DpObjProgramWalker
        Term res = nullptr;
        // already has result
        bool has_res = false;
        void updateRes(Term new_res);
        DpObjCommandWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : IncreCommandWalker(_ctx, _checker) {}
        virtual ~DpObjCommandWalker() = default;
    };
    
    class DpObjProgramWalker: public IncreProgramWalker {
    protected:
        virtual void visit(CommandDef* command);
        virtual void visit(CommandBindTerm* command);
        virtual void visit(CommandDeclare* command);
        virtual void initialize(IncreProgramData* program);
    public:
        // command walker
        DpObjCommandWalker* cmdWalker;
        DpObjProgramWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : cmdWalker(new DpObjCommandWalker(_ctx, _checker)) {}
        ~DpObjProgramWalker() = default;
    };

    // get the type of partial solution using DpObjProgramWalker
    Term getObjFunc(IncreProgramData* program, IncreFullContext ctx);
}

namespace incre::example {
    // store partial solutions and their parent-child relation
    class DpSolutionData;
    typedef std::shared_ptr<DpSolutionData> DpSolution;
    typedef std::vector<DpSolution> DpSolutionList;

    class DpSolutionData {
    public:
        Data partial_solution;
        std::vector<DpSolution> children;
        std::string toString();
        DpSolutionData(Data& _par) : partial_solution(_par) {}
        ~DpSolutionData() = default;
    };
    
    class DpSolutionSet {
    public:
        std::set<std::string> existing_sol;
        DpSolutionList sol_list;
        void add(DpExample& example);
        DpSolution find(Data data);
        DpSolutionSet() = default;
        ~DpSolutionSet() = default;
    };
} // namespace incre::example


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