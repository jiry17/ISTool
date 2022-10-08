//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp_solver.h"

using namespace incre::autolifter;
using solver::autolifter::MaximalInfoList;
using solver::autolifter::EnumerateInfo;

IncrePLPSolver::IncrePLPSolver(PLPInfo *_info, const PProgram &_output_program): plp_info(_info), output_program(_output_program) {
    env = plp_info->o->env;
    examples = plp_info->o->examples;
}
IncrePLPSolver::~IncrePLPSolver() {
    for (auto& info_list: info_storage) {
        for (auto* info: info_list) delete info;
    }
}