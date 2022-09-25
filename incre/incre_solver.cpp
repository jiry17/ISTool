//
// Created by pro on 2022/9/25.
//

#include "istool/incre/incre_solver.h"

using namespace incre;

IncreSolution::IncreSolution(const ProgramList &_f_list, const ProgramList &_tau_list):
    f_list(_f_list), tau_list(_tau_list) {
}
IncreSolver::IncreSolver(IncreInfo *_info): info(_info) {}
