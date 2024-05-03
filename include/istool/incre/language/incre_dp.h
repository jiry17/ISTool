//
// Created by zyw on 2024/3/29.
//

#ifndef ISTOOL_INCRE_DP_H
#define ISTOOL_INCRE_DP_H

#include "incre_program.h"
#include "incre_syntax.h"
#include "incre_types.h"
#include "incre_semantics.h"
#include "istool/basic/grammar.h"
#include "istool/basic/execute_info.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include <set>
#include <queue>

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

    class DpFilterCommandWalker : public IncreCommandWalker {
    protected:
        virtual void walkThroughTerm(Term term);
        virtual void initialize(Command command) {}
        virtual void preProcess(Term term) {}
        virtual void postProcess(Term term) {}
    public:
        std::string r_filter_name;
        std::string r_name;
        DpFilterCommandWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker, std::string _r_filter_name, std::string _r_name) : IncreCommandWalker(_ctx, _checker), r_filter_name(_r_filter_name), r_name(_r_name) {}
        virtual ~DpFilterCommandWalker() = default;
    };
    
    class DpFilterProgramWalker: public IncreProgramWalker {
    protected:
        virtual void visit(CommandDef* command);
        virtual void visit(CommandBindTerm* command);
        virtual void visit(CommandDeclare* command);
        virtual void initialize(IncreProgramData* program);
    public:
        // command walker
        DpFilterCommandWalker* cmdWalker;
        DpFilterProgramWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker, std::string r_filter_name, std::string r_name) : cmdWalker(new DpFilterCommandWalker(_ctx, _checker, r_filter_name, r_name)) {}
        ~DpFilterProgramWalker() = default;
    };

    // add r filter using DpTypeProgramWalker
    incre::IncreProgram addRFilter(IncreProgramData* program, IncreFullContext ctx, std::string r_filter_name, std::string r_name);

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
        // store the type of partial solution, which is the result of DpTypeProgramWalker
        Ty res = nullptr;
        // already has result
        bool has_res = false;
        void updateRes(Ty new_res);
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
        // store the term of main function
        Term main_res = nullptr;
        bool has_main_res = false;
        void updateMainRes(Term new_main_res);
        // command walker
        DpObjCommandWalker* cmdWalker;
        DpObjProgramWalker(IncreFullContext _ctx, incre::types::IncreTypeChecker* _checker) : cmdWalker(new DpObjCommandWalker(_ctx, _checker)) {}
        ~DpObjProgramWalker() = default;
    };

    // get the term of object function using DpObjProgramWalker
    std::pair<Term, Term> getObjFunc(IncreProgramData* program, IncreFullContext& ctx);
    // apply object function on partial solution
    Data applyObjFunc(Term& object_func, Data& sol, incre::semantics::DefaultEvaluator& default_eval, IncreFullContext& ctx);
    // calculate relation of sol_1 and sol_2 on program R
    Data calRelation(PProgram r, Data& sol_1, Data& sol_2, FunctionContext& func_ctx);
} // namespace incre::syntax


namespace grammar {
    // post process of DP program list
    void postProcessDpProgramList(ProgramList& program_list);
    // post process of BOOL program, check whether this program is relative symmetry
    bool postProcessBoolProgram(PProgram program);
    // post process of BOOL program list
    void postProcessBoolProgramList(ProgramList& program_list);
    // post process of DP_BOOL program
    bool postProcessDpBoolProgram(PProgram program);
    // post process of DP_BOOL program list
    void postProcessDpBoolProgramList(ProgramList& program_list);
    // merge DP program and BOOL program to get the full program
    ProgramList mergeProgramList(ProgramList& dp_program_list, ProgramList& bool_program_list);
    // generate height limited program from grammar_original
    std::vector<PProgram> generateHeightLimitedProgram(Grammar* grammar_original, int limit);
    // delete duplicate rule in rule_list
    void deleteDuplicateRule(std::vector<Rule*>& rule_list);
    // delete duplicate rule in all the symbols of this grammar
    void deleteDuplicateRule(Grammar* grammar);
} // namespace grammar

#endif //ISTOOL_INCRE_DP_H