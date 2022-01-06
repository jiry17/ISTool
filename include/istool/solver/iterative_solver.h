//
// Created by pro on 2022/1/5.
//

#ifndef ISTOOL_RELAXABLE_SOLVER_H
#define ISTOOL_RELAXABLE_SOLVER_H

#include "solver.h"

// An interface for a solver in a complex system to iterative enlarge its scope.
class IterativeSolver {
public:
    virtual void* relax(TimeGuard *guard) = 0;
};

namespace solver {
    Solver* relaxSolver(Solver* solver, TimeGuard* guard = nullptr);
    PBESolver* relaxSolver(PBESolver* solver, TimeGuard* guard = nullptr);
}

#endif //ISTOOL_RELAXABLE_SOLVER_H
