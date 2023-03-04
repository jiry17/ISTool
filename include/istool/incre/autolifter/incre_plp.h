//
// Created by pro on 2022/9/27.
//

#ifndef ISTOOL_INCRE_PLP_H
#define ISTOOL_INCRE_PLP_H

#include "istool/basic/grammar.h"
#include "istool/basic/example_space.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/analysis/incre_instru_info.h"

namespace incre::autolifter {
    typedef std::pair<PType, PProgram> TypedProgram;
    typedef std::vector<TypedProgram> TypedProgramList;
    typedef std::pair<TypedProgram, TypedProgram> AuxProgram;
    typedef std::vector<AuxProgram> PLPRes;

    class FExampleSpace {
        void addExample();
    public:
        std::vector<std::pair<std::string, PType>> value_list;
        std::vector<IOExample> example_list;
        Env* env;
        IncreExamplePool* pool;
        int tau_id;
        FExampleSpace(IncreExamplePool* _pool, int _tau_id, const PEnv& _env, AlignTypeInfoData* pass_info);

        Data runAux(int exmaple_id, const AuxProgram& aux);
        std::string example2String(const IOExample& example);
        std::string example2String(int id);
        Data runOup(int example_id, Program* program, const std::vector<int>& path);

        int acquireExample(int target_num, TimeGuard* guard);
    };

    struct TypeLabeledDirectSemantics: public NormalSemantics {
    public:
        PType type;
        TypeLabeledDirectSemantics(const PType& _type);
        virtual Data run(DataList&& inp_list, ExecuteInfo* info);
        virtual ~TypeLabeledDirectSemantics() = default;
    };

    class GrammarEnumerateTool {
        void extend();
    public:
        Grammar* grammar;
        std::vector<TypedProgramList> program_pool;
        int size_limit;
        TypedProgramList* acquirePrograms(int target_size);
        GrammarEnumerateTool(Grammar* _grammar);
        ~GrammarEnumerateTool();
    };

    class PLPTask {
    public:
        FExampleSpace* example_space;
        std::vector<GrammarEnumerateTool*> aux_grammar_list;
        GrammarEnumerateTool* compress_grammar;
        std::vector<TypedProgramList> pre_res_list;
        TypedProgram target;
        std::vector<int> path;

        Data runInp(int example_id, const AuxProgram& program);
        Data runOup(int example_id);
        IOExample getIO(int example_id, const std::vector<AuxProgram>& aux_list);
        int acquireExample(int target_num, int timeout);

        PLPTask(FExampleSpace* _example_space, const std::vector<GrammarEnumerateTool*>& _aux_grammar_list,
                const std::vector<TypedProgramList>& _pre_res,
                GrammarEnumerateTool* _compress_grammar, const TypedProgram& _target, const std::vector<int>& _path);
    };

    Data eliminateCompress(const Data& data);
    Data openLabeledCompress(const Data& data, int label);
    std::string aux2String(const AuxProgram& program);
}

#endif //ISTOOL_INCRE_PLP_H
