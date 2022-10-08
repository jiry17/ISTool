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
    struct FUnitInfo {
        int pos;
        PProgram program;
        Bitset info;
        FUnitInfo(int _pos, const PProgram& _program, const Bitset& _info);
        ~FUnitInfo() = default;
    };

    class IncrePLPSolver {
    public:
        int KComposedNum, KExtraTurnNum;
        Env* env;
        IncreExampleList* examples;
        std::vector<std::pair<int, int>> example_list;
        std::vector<std::pair<PProgram, int>> program_space;
        std::vector<std::vector<solver::autolifter::EnumerateInfo*>> info_storage;
        std::vector<solver::autolifter::MaximalInfoList> maximal_list;
        solver::autolifter::MaximalInfoList global_maximal;

        std::vector<std::queue<solver::autolifter::EnumerateInfo*>> working_list;
        std::unordered_map<std::string, solver::autolifter::EnumerateInfo*> uncovered_info_set;
        int next_component_id;

        PLPInfo* plp_info;
        PProgram output_program;
    public:
        IncrePLPSolver(PLPInfo* _info, const PProgram& _output_program);
        ~IncrePLPSolver();
    };
}

#endif //ISTOOL_INCRE_PLP_SOLVER_H
