//
// Created by pro on 2022/1/5.
//

#ifndef ISTOOL_RELAXABLE_SOLVER_H
#define ISTOOL_RELAXABLE_SOLVER_H

#include "solver.h"

// An interface for a solver in a complex system to iterative enlarge its scope.
class IterativeSolver {
public:
    virtual bool relax() = 0;
};

namespace solver {
    template<class T> bool relaxSolver(T* solver);
}

#endif //ISTOOL_RELAXABLE_SOLVER_H
