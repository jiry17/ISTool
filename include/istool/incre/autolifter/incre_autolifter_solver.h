//
// Created by pro on 2022/9/26.
//

#ifndef ISTOOL_INCRE_AUTOLIFTER_SOLVER_H
#define ISTOOL_INCRE_AUTOLIFTER_SOLVER_H

#include "istool/incre/incre_solver.h"

namespace incre {
    class IncreAutoLifterSolver: public IncreSolver {
    public:
        IncreAutoLifterSolver(IncreInfo* _info);
        virtual IncreSolution solve();
        virtual ~IncreAutoLifterSolver() = default;
    };
}

#endif //ISTOOL_INCRE_AUTOLIFTER_SOLVER_H
