//
// Created by pro on 2022/9/26.
//

#ifndef ISTOOL_INCRE_AUTOLIFTER_SOLVER_H
#define ISTOOL_INCRE_AUTOLIFTER_SOLVER_H

#include "istool/incre/incre_solver.h"
#include "istool/basic/grammar.h"
#include "istool/basic/bitset.h"

namespace incre {
    namespace autolifter {

        typedef std::pair<PType, PProgram> TypedProgram;
        struct FInfo {
            TypedProgram program;
            int id;
            bool is_extended;
            FInfo(const TypedProgram& _program, int _id, bool _is_extended);
        };

        struct FRes {
        private:
            bool isEqual(Program* x, Program* y);
        public:
            std::vector<FInfo> component_list;
            int insert(const TypedProgram& program);
            Data run(const Data& inp, Env* env);
        };

        struct ConstRes {
        private:
            bool isEqual(Program* x, Program* y);
        public:
            std::vector<TypedProgram> const_list;
            int insert(const TypedProgram& program);
        };
        typedef std::pair<std::vector<std::pair<int, TypedProgram>>, std::vector<TypedProgram>> PLPRes;

        struct FExample {
            DataStorage compress_storage;
            DataList const_list;
            Data oup;
            virtual std::string toString() const;
            FExample(const DataStorage& _compress_storage, const DataList& _const_list, const Data& _oup);
        };

        class FExampleSpace {
            void addExample();
        public:
            std::vector<std::pair<std::string, PType>> const_infos;
            std::vector<std::pair<int, std::vector<std::string>>> compress_infos;
            std::vector<FExample> example_list;
            PEnv env;
            IncreExamplePool* pool;
            int tau_id;
            FExampleSpace(IncreExamplePool* _pool, int _tau_id, const PEnv& _env, PassTypeInfoData* pass_info);

            Data runConst(int example_id, Program* program);
            DataList runAux(int example_id, int id, Program* program);
            Data runOup(int example_id, Program* program, const std::vector<int>& path);
            int acquireExample(int target_num, TimeGuard* guard);
        };

        struct OutputUnit {
            std::vector<int> path;
            Ty unit_type;
            OutputUnit(const std::vector<int>& _path, const Ty& _unit_type);
        };

        struct TypeLabeledDirectSemantics: public NormalSemantics {
        public:
            PType type;
            TypeLabeledDirectSemantics(const PType& _type);
            virtual Data run(DataList&& inp_list, ExecuteInfo* info);
            virtual ~TypeLabeledDirectSemantics() = default;
        };

    }
    class IncreAutoLifterSolver: public IncreSolver {
        // Grammar builder
        std::unordered_map<std::string, Grammar*> grammar_map;

        autolifter::PLPRes solvePLPTask(PassTypeInfoData* info, const autolifter::TypedProgram& target, const std::vector<int>& path, int target_id);
        Grammar* buildAuxGrammar(int compress_id);
        Grammar* buildConstGrammar(const TypeList& type_list, int pass_id);
    public:
        Grammar* buildCombinatorGrammar(const TypeList& type_list, const PType& oup_type, int pass_id);

        PEnv env;
        std::vector<autolifter::FExampleSpace*> example_space_list;
        std::vector<std::vector<autolifter::OutputUnit>> unit_storage;

        IncreAutoLifterSolver(IncreInfo* _info, const PEnv& _env);
        virtual ~IncreAutoLifterSolver();
        virtual IncreSolution solve();

        // Synthesize auxiliary programs
        void solveAuxiliaryProgram();
        std::vector<autolifter::FRes> f_res_list;
        TyList f_type_list;
        std::vector<autolifter::ConstRes> const_res_list;

        // Synthesize combinators
        Term synthesisCombinator(int pass_id);
        TermList comb_list;
        void solveCombinators();
    };
}

#endif //ISTOOL_INCRE_AUTOLIFTER_SOLVER_H
