//
// Created by pro on 2022/9/26.
//

#ifndef ISTOOL_INCRE_PLP_SOLVER_H
#define ISTOOL_INCRE_PLP_SOLVER_H

#include "istool/basic/grammar.h"
#include "istool/basic/bitset.h"
#include "incre_plp.h"
#include "istool/solver/autolifter/composed_sf_solver.h"

namespace incre::autolifter {
    struct UnitInfo {
        int pos; // pos = -1 represents const
        PProgram program;
        Bitset info;
        UnitInfo(int _pos, const PProgram& _program, const Bitset& _info);
    };

    class IncrePLPSolver {

        std::string example2String(const std::pair<int, int>& example);


    public:
        int KComposedNum, KExtraTurnNum;
        Env* env;
        PLPTask* task;
        std::vector<std::pair<int, int>> example_list;
        void addExample(const std::pair<int, int>& example);

        // Used to get components
        UnitInfo init(int pos, const PProgram& program);
        void getMoreComponent();
        std::vector<UnitInfo> component_info_list;
        int current_size = 0;
        std::unordered_map<Grammar*, int> grammar_size_map;

        // Used to enumerate compositions
        std::vector<std::vector<solver::autolifter::EnumerateInfo*>> info_storage;
        std::vector<solver::autolifter::MaximalInfoList> maximal_list;
        solver::autolifter::MaximalInfoList global_maximal;
        std::vector<std::queue<solver::autolifter::EnumerateInfo*>> working_list;
        std::unordered_map<std::string, solver::autolifter::EnumerateInfo*> uncovered_info_set;
        int next_component_id = 0;

        // Used to verify
        int verify_num = 0, verify_pos = 0;
        int KVerifyBaseNum, KExampleTimeOut, KExampleEnlargeFactor;
        std::pair<int, int> verify(const std::vector<std::pair<int, PProgram>>& aux_list);

        // Used for synthesis
        bool addUncoveredInfo(solver::autolifter::EnumerateInfo* info);
        void constructInfo(solver::autolifter::EnumerateInfo* info);
        std::pair<solver::autolifter::EnumerateInfo*, solver::autolifter::EnumerateInfo*> recoverResult(
                int pos, solver::autolifter::EnumerateInfo* info);
        std::pair<solver::autolifter::EnumerateInfo*, solver::autolifter::EnumerateInfo*> constructResult(
                solver::autolifter::EnumerateInfo* info, int limit);
        solver::autolifter::EnumerateInfo* getNextComponent(int k, TimeGuard* guard);
        std::vector<std::pair<int, PProgram>> extractResultFromInfo(solver::autolifter::EnumerateInfo* info);
        std::vector<std::pair<int, PProgram>> synthesisFromExample(TimeGuard* guard);

    public:
        IncrePLPSolver(Env* _env, PLPTask* _task);
        PLPRes synthesis(TimeGuard* guard);
    };
}

#endif //ISTOOL_INCRE_PLP_SOLVER_H
