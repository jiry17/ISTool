//
// Created by pro on 2024/3/19.
//

#ifndef ISTOOL_INCRE_ENUMERATION_SOLVER_H
#define ISTOOL_INCRE_ENUMERATION_SOLVER_H

#include "istool/incre/incre_solver.h"

namespace incre {
    class IncreEnumerationSolver {
    public:
        IncreInfo* info;
        PEnv env;
        IncreEnumerationSolver(IncreInfo* _info, const PEnv& _env);
        virtual IncreSolution solve();
        virtual ~IncreEnumerationSolver() = default;
    };
}

#endif //ISTOOL_INCRE_ENUMERATION_SOLVER_H
