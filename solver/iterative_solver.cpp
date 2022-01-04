//
// Created by pro on 2022/1/5.
//

#include "istool/solver/iterative_solver.h"

template<class T> bool solver::relaxSolver(T *solver) {
    auto* it = dynamic_cast<IterativeSolver*>(solver);
    if (it) return it->relax();
    return false;
}