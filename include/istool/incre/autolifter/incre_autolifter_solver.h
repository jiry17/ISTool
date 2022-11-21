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
        struct FInfo {
            PProgram program;
            int id;
            bool is_extended;
            FInfo(const PProgram& _program, int _id, bool _is_extended);
        };

        struct FRes {
        private:
            bool isEqual(Program* x, Program* y);
        public:
            std::vector<FInfo> component_list;
            int insert(const PProgram& program);
        };

        struct ConstRes {
        private:
            bool isEqual(Program* x, Program* y);
        public:
            ProgramList const_list;
            int insert(const PProgram& program);
        };

        typedef std::pair<std::vector<std::pair<int, PProgram>>, ProgramList> PLPRes;

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

        struct CombinatorCase {
            Bitset null_inps;
            PProgram program;
            CombinatorCase(const Bitset& _null_inputs, const PProgram& _program);
        };

        typedef std::vector<CombinatorCase> CombinatorRes;
    }
    class IncreAutoLifterSolver: public IncreSolver {
        autolifter::PLPRes solvePLPTask(PassTypeInfoData* info, const PProgram& target, const std::vector<int>& path, int target_id);
        Grammar* buildAuxGrammar(int compress_id);
        Grammar* buildConstGrammar(const TypeList& type_list);
        Grammar* buildCombinatorGrammar(const TypeList& type_list);
        Grammar* buildCombinatorCondGrammar(const TypeList& type_list);
    public:
        PEnv env;
        std::vector<autolifter::FExampleSpace*> example_space_list;
        std::vector<std::vector<autolifter::OutputUnit>> unit_storage;

        IncreAutoLifterSolver(IncreInfo* _info, const PEnv& _env);
        virtual ~IncreAutoLifterSolver();
        std::vector<autolifter::FRes> f_res_list;
        std::vector<autolifter::ConstRes> const_res_list;
        virtual IncreSolution solve();

        // Synthesize auxiliary programs
        void solveAuxiliaryProgram();

        // Synthesize combinators
        autolifter::CombinatorRes synthesisCombinator(int pass_id);
        void solveCombinators();


    };
}

#endif //ISTOOL_INCRE_AUTOLIFTER_SOLVER_H
